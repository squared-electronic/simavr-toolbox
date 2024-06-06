#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "sim_avr.h"
#include "sim_i2c_base.hpp"
#include "sim_irq.h"

class SimGu7000 final : public SimAvrI2CComponent {
 public:
  SimGu7000(avr_t* avr, avr_irq_t* busyPin, uint8_t i2cAddress);
  virtual void HandleI2CMessage(const avr_twi_msg_t& msg) override;
  virtual void ResetStateMachine() override;
  const std::string& CurrentDisplay() const;

 private:
  avr_irq_t* BusyPin_{nullptr};
  void ExecuteCommand();
  void PulseBusyPin();
  bool MaybeBusy_{false};
  std::vector<uint8_t> CommandBuffer_;
  std::string Display_;
  uint8_t Cursor = 0;
};