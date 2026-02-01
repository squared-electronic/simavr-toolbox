#include "sim_i2c_base.hpp"

#include <cstdint>

void FakeI2cCb(struct avr_irq_t* irq, uint32_t value, void* param) {
  auto cb = (I2cMessageCallback*)param;
  (*cb)(value);
}

const char* SimAvrI2CComponent::irq_names[MyIrqType::Count] = {
    [MyIrqType::Input] = "I2CIn",
    [MyIrqType::Output] = "I2COut",
};

bool SimAvrI2CComponent::MatchesI2cAddress(const avr_twi_msg_t& message, uint8_t i2cAddress) {
  // Don't check the LSB which is W/R status
  return (message.addr >> 1) == i2cAddress;
}

SimAvrI2CComponent::SimAvrI2CComponent(avr_t* avr, uint8_t i2cAddressRightShifted)
    : SimAvrI2CComponent(avr, i2cAddressRightShifted, [i2cAddressRightShifted](avr_twi_msg_t* msg) {
        return MatchesI2cAddress(*msg, i2cAddressRightShifted);
      }) {}

SimAvrI2CComponent::SimAvrI2CComponent(avr_t* avr, uint8_t i2cAddressRightShifted,
                                       I2cAddressMatcher addressMatcher)
    : Avr_(avr), I2cAddress_{i2cAddressRightShifted}, AddressMatcher_(addressMatcher) {
  i2c_message_callback_ =
      std::bind(&SimAvrI2CComponent::HandleAnyI2cMessage, this, std::placeholders::_1);

  MyIrqs_ = avr_alloc_irq(&avr->irq_pool, 0, MyIrqType::Count, irq_names);

  MyOutputIrq_ = &MyIrqs_[MyIrqType::Output];

  avr_connect_irq(avr_io_getirq(Avr_, AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_OUTPUT),
                  &MyIrqs_[MyIrqType::Input]);

  avr_connect_irq(&MyIrqs_[MyIrqType::Output],
                  avr_io_getirq(Avr_, AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_INPUT));

  avr_irq_register_notify(&MyIrqs_[MyIrqType::Input], FakeI2cCb, &i2c_message_callback_);
}

SimAvrI2CComponent::~SimAvrI2CComponent() {
  avr_unconnect_irq(avr_io_getirq(Avr_, AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_OUTPUT),
                    &MyIrqs_[MyIrqType::Input]);
  avr_unconnect_irq(&MyIrqs_[MyIrqType::Output],
                    avr_io_getirq(Avr_, AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_INPUT));
  avr_free_irq(MyIrqs_, MyIrqType::Count);
}

void SimAvrI2CComponent::SendToAvrI2CAck() {
  avr_raise_irq(MyOutputIrq_, avr_twi_irq_msg(TWI_COND_ACK, I2cAddress_, 1));
}

void SimAvrI2CComponent::SendByteToAvrI2c(uint8_t byte) {
  avr_raise_irq(MyOutputIrq_, avr_twi_irq_msg(TWI_COND_ACK | TWI_COND_READ, I2cAddress_, byte));
}

avr_twi_msg_t SimAvrI2CComponent::Parseavr_twi_msg_t(uint32_t value) {
  avr_twi_msg_irq_t v;
  v.u.v = value;
  return v.u.twi;
}

void SimAvrI2CComponent::HandleAnyI2cMessage(uint32_t value) {
  auto msg = Parseavr_twi_msg_t(value);

  if (!AddressMatcher_(&msg)) {
    // A start for a different device on the bus triggers a reset for this
    // device.
    if (msg.msg & TWI_COND_START) {
      ResetStateMachine();
    }
    return;
  }

  if (msg.msg & TWI_COND_STOP) {
    ResetStateMachine();
  }

  HandleI2CMessage(msg);
}

void SimAvrI2CComponent::ResetStateMachine() {
  // Do nothing.
}