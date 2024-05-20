#pragma once

#include <cstdint>
#include <functional>

#include "sim_avr.h"
#include "sim_irq.h"

class SimTLP9202 {
 public:
  SimTLP9202(avr_t* avr, avr_irq_t* csPin);
  uint8_t GetCurrentValue() const;

 private:
  using Fn = std::function<void(uint8_t)>;
  static void IrqCb(struct avr_irq_t* irq, uint32_t value, void* param);
  void OnSpiData(uint8_t Data);
  bool LastDataBit_{false};
  uint8_t Value_{0};
  uint8_t LatchedData_{0};
  avr_irq_t* CsIrq_{nullptr};
  Fn SpiCb_;
};