#pragma once

#include <simavr/avr_twi.h>
#include <simavr/sim_avr.h>
#include <simavr/sim_irq.h>

#include <cstdint>
#include <functional>

using I2cMessageCallback = std::function<void(uint32_t)>;
using IrqCallback = std::function<avr_cycle_count_t(avr_cycle_count_t when)>;

class SimAvrI2CComponent {
 public:
  SimAvrI2CComponent(avr_t *avr, uint8_t i2cAddress);
  ~SimAvrI2CComponent();
  virtual void HandleI2CMessage(avr_twi_msg_t msg) = 0;
  virtual void Reset() = 0;

 protected:
  avr_t *Avr_{nullptr};
  enum class I2CMode { WRITE = 0, READ };
  void SendToAvrI2CAck();
  void SendByteToAvrI2c(uint8_t byte);
  void RegisterTimer(IrqCallback, avr_cycle_count_t when);

 private:
  avr_twi_msg_t Parseavr_twi_msg_t(uint32_t value);
  bool MatchesOurAddress(const avr_twi_msg_t &message) const;
  void HandleAnyI2cMessage(uint32_t value);

  enum MyIrqType { Input = 0, Output, Count };
  static const char *irq_names[MyIrqType::Count];
  avr_irq_t *MyIrqs_{nullptr};
  avr_irq_t *MyOutputIrq_{nullptr};
  uint8_t I2cAddress_{0};
  I2cMessageCallback i2c_message_callback_;
};