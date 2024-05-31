#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "sim_avr.h"
#include "sim_i2c_base.hpp"

class SimTLC59116 final : public SimAvrI2CComponent {
 public:
  SimTLC59116(avr_t* avr, uint8_t i2cAddress);
  virtual void HandleI2CMessage(avr_twi_msg_t msg) override;
  virtual void Reset() override;
  std::array<uint8_t, 16> GetCurrentState() const;

 private:
  enum class State {
    Start,
    GiveRegisterAddress,
    ReadRegister,
    WriteRegister,
  } State_;
  std::vector<uint8_t> CommandBuffer_;
  std::array<uint8_t, 0x1E> Registers_;
  uint8_t SelectedRegister_;
  bool AutoIncrement_;
};