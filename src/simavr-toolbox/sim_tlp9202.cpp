#include "sim_tlp9202.hpp"

#include <simavr/avr_spi.h>

#include "sim_io.h"
#include "sim_irq.h"

SimTLP9202::SimTLP9202(avr_t* avr, avr_irq_t* csPin) {
  CsIrq_ = csPin;
  SpiCb_ = std::bind(&SimTLP9202::OnSpiData, this, std::placeholders::_1);
  auto spiOut = avr_io_getirq(avr, AVR_IOCTL_SPI_GETIRQ(0), SPI_IRQ_OUTPUT);
  avr_irq_register_notify(spiOut, IrqCb, &SpiCb_);
}

void SimTLP9202::OnSpiData(uint8_t Data) {
  if (!CsIrq_->value) {
    LatchedData_ = Data;
  }
}

const uint8_t SimTLP9202::GetCurrentValue() const {
  return LatchedData_;
}

void SimTLP9202::IrqCb(struct avr_irq_t* irq, uint32_t value, void* param) {
  auto cb = (Fn*)param;
  (*cb)(value);
}