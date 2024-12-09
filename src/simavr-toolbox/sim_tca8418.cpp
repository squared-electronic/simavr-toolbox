#include "sim_tca8418.hpp"

#include <cstdint>
#include <cstdlib>
#include <functional>

#include "avr_twi.h"
#include "sim_base.hpp"
#include "sim_irq.h"
#include "sim_time.h"

avr_cycle_count_t CallLater(struct avr_t* avr, avr_cycle_count_t when, void* param) {
  auto cb = (std::function<void()>*)param;
  (*cb)();
  return 0;
}

SimTca8418::SimTca8418(avr_t* avr, avr_irq_t* intIrq)
    : SimAvrI2CComponent(avr, I2C_ADDRESS), Avr_(avr), AvrIntIrq_(intIrq) {
  // "The default value in all registers is 0"
  Registers_.fill(0);
}

void SimTca8418::HandleI2CMessage(const avr_twi_msg_t& message) {
  if (message.msg & TWI_COND_START) {
    if (State_ == State::Start) {
      auto mode = static_cast<I2CMode>(message.addr & 1);
      if (mode == I2CMode::WRITE) {
        State_ = State::GiveRegisterAddress;
      } else {
        std::abort();
      }
    } else if (State_ == State::MaybeWriteOrReadRegister) {
      // Repeated start, to read register
      State_ = State::ReadRegister;
    } else {
      std::abort();
    }
    SendToAvrI2CAck();
  } else if (message.msg & TWI_COND_STOP) {
    ResetStateMachine();
  } else if (message.msg & TWI_COND_WRITE) {
    SendToAvrI2CAck();
    if (State_ == State::GiveRegisterAddress) {
      SelectedRegister_ = message.data;
      State_ = State::MaybeWriteOrReadRegister;
    } else if (State_ == State::MaybeWriteOrReadRegister) {
      // They decided to write a register, do it
      uint8_t oldData = Registers_.at(SelectedRegister_);
      Registers_.at(SelectedRegister_) = message.data;
      CheckSpecialCaseWrite(static_cast<register_t>(SelectedRegister_), oldData, message.data);
      // sim_debug_log("Wrote Register %02x = %02X\n", SelectedRegister_, message.data);
      IncrementSelectedRegisterMaybe();
    }
  } else if (message.msg & TWI_COND_READ) {
    if (State_ == State::ReadRegister) {
      SendByteToAvrI2c(Registers_.at(SelectedRegister_));
      // sim_debug_log("Read Register %02x = %02X\n", SelectedRegister_,
      // Registers_.at(SelectedRegister_));
      CheckSpecialCaseReads(static_cast<register_t>(SelectedRegister_));
      IncrementSelectedRegisterMaybe();
    } else {
      std::abort();
    }
  }
}

void SimTca8418::ResetStateMachine() {
  State_ = State::Start;
  SelectedRegister_ = 0;
}

void SimTca8418::AddKeyEvent(Event ev, uint8_t row, uint8_t col) {
  uint8_t msb = static_cast<uint8_t>(ev);
  uint8_t newEvent = msb | ((row * 10) + (col + 1));
  AddKeyRawEvent(newEvent);
}

void SimTca8418::AddKeyPressAndRelease(uint8_t keyCode) {
  AddKeyPress(keyCode);

  auto releaseCode = static_cast<uint8_t>(Event::Release) | keyCode;
  auto releaseCb = std::bind(&SimTca8418::AddKeyRawEventAndClearPending, this, releaseCode);
  PendingPresses_.emplace(keyCode, releaseCb);

  auto when = avr_usec_to_cycles(Avr_, 200000);
  avr_cycle_timer_register(Avr_, when, CallLater, &PendingPresses_[keyCode]);
}

void SimTca8418::AddKeyPress(uint8_t rawKeyCode) {
  auto pressCode = rawKeyCode | static_cast<uint8_t>(Event::Press);
  AddKeyRawEvent(pressCode);
}

void SimTca8418::AddKeyRelease(uint8_t rawKeyCode) {
  auto pressCode = rawKeyCode | static_cast<uint8_t>(Event::Release);
  AddKeyRawEvent(pressCode);
}

void SimTca8418::IncrementSelectedRegisterMaybe() {
  // If Register w/r auto-increment is enabled in control register
  if ((Registers_.at(0x01) & 0x80) == 0x80) {
    // Increments to max value of 0x2E; 0x2F is reserved, and it wraps around to 0, even though 0
    // is reserved.
    if (SelectedRegister_ == 0x2E) {
      SelectedRegister_ = 0;
    } else {
      SelectedRegister_ += 1;
    }
  }
}

void SimTca8418::AddKeyRawEventAndClearPending(uint8_t rawKeyCode) {
  AddKeyRawEvent(rawKeyCode);
  PendingPresses_.erase(rawKeyCode);
}

void SimTca8418::AddKeyRawEvent(uint8_t rawKeyCode) {
  uint8_t eventCount = Registers_.at(register_t::KEY_LCK_EC) & 0x0F;

  if (eventCount == 10) {
    // TODO: Handle overflow
    auto statReg = Registers_.at(register_t::INT_STAT);
    if (statReg & (1 << 5) && statReg & (1 << 3)) {
    }
  }

  uint8_t nextReg = register_t::KEY_EVENT_A + eventCount;
  Registers_.at(nextReg) = rawKeyCode;

  ModifyRegister(register_t::KEY_LCK_EC, eventCount + 1, 0x0F);

  if (Registers_.at(register_t::CFG) & (1 << 0)) {
    UnacknowledgedInts_ |= 1;
    ModifyRegister(register_t::INT_STAT, 0x01, 0x01);
    avr_raise_irq(AvrIntIrq_, 0);
  }
}

void SimTca8418::CheckSpecialCaseWrite(register_t reg, uint8_t oldValue, uint8_t newValue) {
  if (reg == register_t::INT_STAT) {
    if (UnacknowledgedInts_ != 0) {
      UnacknowledgedInts_ &= ~newValue;
      // If no more pending interrupts, de-assert the INT line.
      // TODO, this is assuming only the "constant low INT if pending",
      // not the "toggle the int flag if pending" mode.
      if (UnacknowledgedInts_ == 0) {
        avr_raise_irq(AvrIntIrq_, 1);
      }
    }
  }
}

void SimTca8418::CheckSpecialCaseReads(register_t reg) {
  if (reg == register_t::KEY_EVENT_A) {
    OnFifoRead();
  }
}

void SimTca8418::OnFifoRead() {
  uint8_t eventCount = Registers_.at(register_t::KEY_LCK_EC) & 0x0F;

  if (eventCount == 0) {
    return;
  }

  for (uint8_t i = 0; i < eventCount - 1; ++i) {
    Registers_.at(register_t::KEY_EVENT_A + i) = Registers_.at(register_t::KEY_EVENT_A + i + 1);
  }

  ModifyRegister(register_t::KEY_LCK_EC, eventCount - 1, 0x0F);
}

void SimTca8418::ModifyRegister(register_t register_address, uint8_t data, uint8_t mask) {
  uint8_t originalData = Registers_.at(register_address);

  uint8_t newData = originalData;
  newData &= ~mask;
  newData |= (data & mask);

  Registers_.at(register_address) = newData;
}