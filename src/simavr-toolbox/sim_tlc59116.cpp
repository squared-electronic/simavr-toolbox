#include "sim_tlc59116.hpp"

#include <cstdlib>
#include <iostream>
#include <ostream>

#include "avr_twi.h"
#include "sim_i2c_base.hpp"
#include "sim_irq.h"
#include "sim_time.h"

SimTLC59116::SimTLC59116(avr_t* avr, uint8_t i2cAddress) : SimAvrI2CComponent(avr, i2cAddress) {}

void SimTLC59116::HandleI2CMessage(avr_twi_msg_t msg) {
  if (msg.msg & TWI_COND_STOP) {
    ExecuteCommand();
  } else if (msg.msg & TWI_COND_START) {
    CommandBuffer_.clear();
  } else if (msg.msg & TWI_COND_WRITE) {
    CommandBuffer_.push_back(msg.data);
  } else {
    std::abort();
  }
}

void SimTLC59116::ExecuteCommand() {
  // use CommandBuffer_
}

void SimTLC59116::Reset() {
  CommandBuffer_.clear();
}