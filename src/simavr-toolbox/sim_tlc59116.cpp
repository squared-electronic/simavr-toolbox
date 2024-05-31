#include "sim_tlc59116.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>

#include "avr_twi.h"
#include "sim_base.hpp"
#include "sim_i2c_base.hpp"

SimTLC59116::SimTLC59116(avr_t* avr, uint8_t i2cAddress) : SimAvrI2CComponent(avr, i2cAddress) {
  Registers_.fill(0);
}

void SimTLC59116::HandleI2CMessage(avr_twi_msg_t message) {
  if (message.msg & TWI_COND_START) {
    if (State_ == State::Start) {
      auto mode_ = static_cast<I2CMode>(message.addr & 1);
      if (mode_ == I2CMode::WRITE) {
        State_ = State::GiveRegisterAddress;
      } else {
        std::abort();
      }
    } else {
      std::abort();
    }
    SendToAvrI2CAck();
  } else if (message.msg & TWI_COND_STOP) {
    Reset();
  } else if (message.msg & TWI_COND_WRITE) {
    SendToAvrI2CAck();
    if (State_ == State::GiveRegisterAddress) {
      AutoIncrement_ = (message.data & 0xE0) != 0;  // TODO support other AI modes
      SelectedRegister_ = message.data & 0x1F;
      State_ = State::WriteRegister;
    } else if (State_ == State::WriteRegister) {
      // They decided to write a register, do it
      Registers_.at(SelectedRegister_) = message.data;
      if (AutoIncrement_) {
        SelectedRegister_ += 1;
        // TODO support other AI modes
        if (SelectedRegister_ > 0x1B) {
          SelectedRegister_ = 0;
        }
      }
      debug_log("Wrote Register %02x = %02X\n", SelectedRegister_, message.data);
    }
  } else if (message.msg & TWI_COND_READ) {
    std::abort();
  }
}

void SimTLC59116::Reset() {
  CommandBuffer_.clear();
  AutoIncrement_ = false;
  SelectedRegister_ = 0;
}

enum class LedState : uint8_t {
  Off = 0,
  FullOn = 1,
  Pwm = 2,
  PwmGroup = 3,
};

std::array<uint8_t, 16> SimTLC59116::GetCurrentState() const {
  std::array<uint8_t, 16> value;

  for (int i = 0; i < 16; ++i) {
    const uint8_t reg = i / 4;
    const uint8_t bit = i % 4;
    auto reg_2 = (Registers_.at(0x14 + reg) >> (2 * bit)) & 0x03;
    const auto status = static_cast<LedState>(reg_2);
    switch (status) {
      case LedState::Off:
        value[i] = 0;
        break;
      case LedState::FullOn:
        value[i] = 1;
        break;
      case LedState::Pwm:
        value[i] = Registers_.at(0x02 + i);
        break;
      case LedState::PwmGroup:
        std::abort();
        // value[i] = Registers_.at(0x02 + 1);
        break;
    }
  }

  return value;
}
