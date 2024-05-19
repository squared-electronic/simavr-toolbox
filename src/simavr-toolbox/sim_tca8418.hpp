#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <map>

#include "sim_i2c_base.hpp"
#include "sim_irq.h"

class SimTca8418 : public SimAvrI2CComponent {
 public:
  static constexpr uint8_t I2C_ADDRESS = 0x68;
  SimTca8418(avr_t* avr, avr_irq_t* intIrq);
  virtual void HandleI2CMessage(avr_twi_msg_t message) override;
  virtual void Reset() override;
  enum class Event { Release = 0x00, Press = 0x80 };
  void AddKeyEvent(Event ev, uint8_t row, uint8_t col);
  void AddKeyEventCode(Event ev, uint8_t row, uint8_t col);
  void AddKeyPressAndRelease(uint8_t rawKeyCode);

 private:
  enum register_t : uint8_t {
    CFG = 0x01,
    INT_STAT = 0x02,
    KEY_LCK_EC = 0x03,
    KEY_EVENT_A = 0x04,
    KEY_EVENT_J = 0x0D,
    KP_GPIO1 = 0x1D,
    KP_GPIO2 = 0x1E,
    KP_GPIO3 = 0x1F,
    GPIO_INT_EN1 = 0x1A,
    GPIO_INT_EN2 = 0x1B,
    GPIO_INT_EN3 = 0x1C,
    GPIO_EM1 = 0x20,
    GPIO_EM2 = 0x21,
    GPIO_EM3 = 0x22,
    GPIO_DIR1 = 0x23,
    GPIO_DIR2 = 0x24,
    GPIO_DIR3 = 0x25,
    GPIO_INT_LVL1 = 0x26,
    GPIO_INT_LVL2 = 0x27,
    GPIO_INT_LVL3 = 0x28,
    GPIO_PULL1 = 0x2C,
    GPIO_PULL2 = 0x2D,
    GPIO_PULL3 = 0x2E,
  };
  void AddKeyRawEventAndClearPending(uint8_t rawKeyCode);
  void AddKeyRawEvent(uint8_t rawKeyCode);
  void CheckSpecialCaseWrite(register_t reg, uint8_t oldData, uint8_t newData);
  void CheckSpecialCaseReads(register_t reg);
  void OnFifoRead();
  void ModifyRegister(register_t register_address, uint8_t data, uint8_t mask);
  void IncrementSelectedRegisterMaybe();
  enum class State {
    Start,
    GiveRegisterAddress,
    MaybeWriteOrReadRegister,
    ReadRegister,
    WriteRegister,
  };
  State State_;
  uint8_t SelectedRegister_{0};
  uint8_t UnacknowledgedInts_{0};
  std::array<uint8_t, 0x2F> Registers_;
  avr_t* Avr_{nullptr};
  avr_irq_t* AvrIntIrq_{nullptr};
  using Cb = std::function<void()>;
  std::map<uint8_t /* key code */, Cb> PendingPresses_;
};