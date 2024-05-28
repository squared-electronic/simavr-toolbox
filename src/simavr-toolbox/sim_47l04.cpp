

#include "sim_47l04.h"

#include <cstdint>
#include <string>
#include <thread>

#include "avr_twi.h"

// 24LCXX control code.
constexpr uint8_t kControlCode = 0b10100000;

#include "sim_base.hpp"

std::string GetMessageType(const avr_twi_msg_t &message) {
  std::string str;
  if (message.msg & TWI_COND_STOP) {
    str += "STOP";
  }
  if (message.msg & TWI_COND_START) {
    str += "START";
  }
  if (message.msg & TWI_COND_WRITE) {
    str += "WRITE";
  }
  if (message.msg & TWI_COND_READ) {
    str += "READ";
  }
  return str;
}

bool IsControlByte(const avr_twi_msg_t &message) {
  return message.addr & kControlCode;
}

uint8_t MakeAddress(bool a2, bool a1) {
  uint8_t mask = (a2 << 1) | (a1);
  mask <<= 2;
  return kControlCode | mask;
}

I2cEeprom::I2cEeprom(avr_t *avr, bool a2, bool a1) : SimAvrI2CComponent(avr, MakeAddress(a2, a1)) {
  Reset();
}

void I2cEeprom::HandleI2CMessage(avr_twi_msg_t message) {
  std::this_thread::sleep_for(std::chrono::milliseconds(5));

  if (message.msg & TWI_COND_STOP) {
    state_ = STOPPED;
    Reset();
  } else if (message.msg & TWI_COND_START) {
    if (!IsControlByte(message)) {
      // This should never be invoked in the threeboard code.
      debug_log(
          "SIM::HandleI2cMessage: Received non-control start message, "
          "returning\n");
      return;
    }

    // ACK the start message, parse out the read/write flag, and prepare to
    // receive the two address bytes.
    mode_ = static_cast<I2CMode>(message.addr & 1);

    debug_log("SIM::HandleI2cMessage: Received control byte, mode=%s\n",
              mode_ == I2CMode::READ ? "READ" : "WRITE");

    // The address word should only be sent for write operations. Read
    // operations are addressed first by starting a write operation and writing
    // the address word, and then sending a repeated start to switch the device
    // into read operation mode once it has been addressed.
    if (mode_ == I2CMode::WRITE) {
      state_ = ADDRESSING_HIGH;
    } else {
      state_ = STARTED;
    }

    SendToAvrI2CAck();
  } else if (message.msg & TWI_COND_WRITE) {
    // Ack the message.
    SendToAvrI2CAck();

    // Write messages can serve one of two purposes:
    // 1. Writing the address of the read/write operation.
    // 2. Writing data to the EEPROM as part of an already addressed write
    //    operation.
    // Address writing always happens before any write occurs.
    if (state_ == ADDRESSING_HIGH) {
      operation_address_ |= (message.data << 8);
      debug_log("Address high: 0x%02x\n", operation_address_);
      state_ = ADDRESSING_LOW;
    } else if (state_ == ADDRESSING_LOW) {
      operation_address_ |= message.data;
      debug_log("Address low: 0x%02x\n", operation_address_);
      state_ = STARTED;
    } else if (state_ == STARTED) {
      debug_log("Started: 0x%02x 0x%02x\n", operation_address_ + operation_address_counter_,
                message.data);
      (buffer_)[operation_address_ + operation_address_counter_] = message.data;
      operation_address_counter_++;
    }
  } else if (message.msg & TWI_COND_READ) {
    // Simple return of selected byte.
    uint8_t current_byte = (buffer_)[operation_address_ + operation_address_counter_];
    debug_log("SIM::HandleI2cMessage: Responding to read at addr 0x%02x with byte %d\n",
              operation_address_ + operation_address_counter_, current_byte);
    operation_address_counter_++;
    SendByteToAvrI2c(current_byte);
  }
}

void I2cEeprom::Reset() {
  operation_address_ = 0;
  operation_address_counter_ = 0;
  state_ = STOPPED;
}