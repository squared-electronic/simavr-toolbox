#include "sim_gu7000.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <string>

#include "avr_twi.h"
#include "sim_base.hpp"
#include "sim_i2c_base.hpp"
#include "sim_irq.h"
#include "sim_time.h"
#include "timer.hpp"

SimGu7000::SimGu7000(avr_t* avr, avr_irq_t* busyPin, uint8_t i2cAddress)
    : SimAvrI2CComponent(avr, i2cAddress), BusyPin_{busyPin} {
  ResetStateMachine();
}

void SimGu7000::HandleI2CMessage(const avr_twi_msg_t& msg) {
  if (MaybeBusy_) {
    // Debug to alert doing anything when busy pin was 1
    std::cout << "Write to display when busy" << std::endl;
    std::abort();
  }

  if (msg.msg & TWI_COND_STOP) {
    // PulseBusyPin(); Currently crashes SimAVR -  maybe clever timer setup
    ExecuteCommand();
  } else if (msg.msg & TWI_COND_START) {
    CommandBuffer_.clear();
    SendToAvrI2CAck();
  } else if (msg.msg & TWI_COND_WRITE) {
    CommandBuffer_.push_back(msg.data);
    SendToAvrI2CAck();
  } else {
    std::abort();
  }
}

void SimGu7000::PulseBusyPin() {
  MaybeBusy_ = true;

  auto startTime = avr_usec_to_cycles(Avr_, 20);
  auto endTime = avr_usec_to_cycles(Avr_, 60);

  RegisterTimer(
      Avr_,
      [this](auto when) {
        avr_raise_irq(BusyPin_, 1);
        return 0;
      },
      startTime);

  RegisterTimer(
      Avr_,
      [this](auto when) {
        avr_raise_irq(BusyPin_, 0);
        MaybeBusy_ = false;
        return 0;
      },
      endTime);
}

const char* get_escape_sequence(char ch) {
  switch (ch) {
    case '\a':
      return "\\a";
    case '\b':
      return "\\b";
    case '\f':
      return "\\f";
    case '\n':
      return "\\n";
    case '\r':
      return "\\r";
    case '\t':
      return "\\t";
    case '\v':
      return "\\v";
    case '\\':
      return "\\\\";
    case '\"':
      return "\\\"";
    case '\'':
      return "\\\'";
    default:
      return NULL;
  }
}

void SimGu7000::ExecuteCommand() {
  std::string s;
  s.reserve(CommandBuffer_.size());
  for (const auto c : CommandBuffer_) {
    auto x = get_escape_sequence(c);
    if (x) {
      s += x;
    } else {
      s += c;
    }
  }
  sim_debug_log("Screen: \"%s\"", s.data());
  // if (CommandBuffer_.empty()) {
  //   // sim_debug_log("Warning: Empty Command Buffer");
  // } else if (CommandBuffer_.size() == 1) {
  //   auto b = CommandBuffer_.front();
  //   if (b >= 0x20 && b <= 0xFF) {
  //     // Display Character
  //     Display_.at(Cursor) = b;
  //     Cursor += 1;
  //   } else if (b == 0x08) {
  //     // Backspace
  //     if (Cursor > 0) {
  //       Cursor -= 1;
  //     }
  //   } else if (b == 0x0B) {
  //     // Home Position
  //     Cursor = 0;
  //   } else if (b == 0x0C) {
  //     // Display Clear
  //     std::fill(Display_.begin(), Display_.end(), ' ');
  //   } else if (b == 0x09) {
  //     // Horizontal Tab
  //     Cursor += 1;
  //   } else {
  //     sim_debug_log("Warning: Unknown Screen Command");
  //   }
  // } else if (CommandBuffer_.size() == 2) {
  //   auto a = CommandBuffer_[0];
  //   auto b = CommandBuffer_[1];
  //   if (a == 0x1B && b == 0x40) {
  //     // Initialize Display
  //     std::fill(Display_.begin(), Display_.end(), ' ');
  //     Cursor = 0;
  //   } else {
  //     sim_debug_log("Warning: Unknown Screen Command");
  //   }
  // } else {
  //   sim_debug_log("Warning: Unknown Screen Command");
  // }
}

void SimGu7000::ResetStateMachine() {
  // Nothing to do here.
}

const std::string& SimGu7000::CurrentDisplay() const {
  return Display_;
}