#include "sim_gu7000_i2c.hpp"

SimGu7000I2C::SimGu7000I2C(avr_t* avr) : SimAvrI2CSmarterComponent(avr, 0x50) {}

const SimGu7000::DisplayMemory& SimGu7000I2C::GetDisplayMemory() const {
  return screen_.GetDisplayMemory();
}

void SimGu7000I2C::OnMillisecondPassed() {
  if (screen_dirty_) {
    last_command_debounce_ms_ += 1;
  }
}

void SimGu7000I2C::CleanScreen() {
  screen_dirty_ = false;
}

void SimGu7000I2C::OnDataReceived(const std::vector<uint8_t>& data) {
  last_command_debounce_ms_ = 0;
  screen_dirty_ = true;
  for (auto byte : data) {
    screen_.ProcessCommand(byte);
  }
}
