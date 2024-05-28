#include "sim_gu7000.hpp"

#include <cstdlib>
#include <iostream>
#include <ostream>

#include "avr_twi.h"
#include "sim_i2c_base.hpp"
#include "sim_irq.h"
#include "sim_time.h"

SimGu7000::SimGu7000(avr_t* avr, avr_irq_t* busyPin, uint8_t i2cAddress)
    : SimAvrI2CComponent(avr, i2cAddress), BusyPin_{busyPin} {}

void SimGu7000::HandleI2CMessage(avr_twi_msg_t msg) {
  if (MaybeBusy_) {
    // Debug to alert doing anything when busy pin was 1
    std::cout << "Write to display when busy" << std::endl;
    std::abort();
  }

  if (msg.msg & TWI_COND_STOP) {
    ExecuteCommand();
  } else if (msg.msg & TWI_COND_START) {
    CommandBuffer_.clear();
  } else if (msg.msg & TWI_COND_WRITE) {
    CommandBuffer_.push_back(msg.data);
    PulseBusyPin();
  } else {
    std::abort();
  }
}

void SimGu7000::PulseBusyPin() {
  MaybeBusy_ = true;

  auto startTime = avr_usec_to_cycles(Avr_, 20);
  auto endTime = avr_usec_to_cycles(Avr_, 60);

  RegisterTimer(
      [this](auto when) {
        avr_raise_irq(BusyPin_, 1);
        return 0;
      },
      startTime);

  RegisterTimer(
      [this](auto when) {
        avr_raise_irq(BusyPin_, 0);
        MaybeBusy_ = false;
        return 0;
      },
      endTime);
}

void SimGu7000::ExecuteCommand() {
  // use CommandBuffer_
}

void SimGu7000::Reset() {
  CommandBuffer_.clear();
}