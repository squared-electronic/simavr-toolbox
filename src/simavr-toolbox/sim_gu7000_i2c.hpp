#pragma once

#include <cstdint>

#include "sim_gu7000.hpp"
#include "sim_i2c_smarter_base.hpp"

class SimGu7000I2C : public SimAvrI2CSmarterComponent {
 public:
  SimGu7000I2C(avr_t* avr);
  const SimGu7000::DisplayMemory& GetDisplayMemory() const;
  void OnMillisecondPassed();
  void CleanScreen();

 private:
  void OnDataReceived(const std::vector<uint8_t>& data) override;

  SimGu7000 screen_;
  uint64_t last_command_debounce_ms_{0};
  bool screen_dirty_{false};
};
