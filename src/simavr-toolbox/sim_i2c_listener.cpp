#include "sim_i2c_listener.hpp"

#include <simavr/avr_twi.h>

#include <cstdint>
#include <simavr-toolbox/sim_base.hpp>

void FakeI2cCb2(struct avr_irq_t* irq, uint32_t value, void* param) {
  avr_twi_msg_irq_t msg;
  msg.u.v = value;
  auto cb = (I2cStructMessageCallback*)param;
  (*cb)(&msg);
}

SimI2CListener::SimI2CListener(avr_t* avr) {
  OnMessageFromAvr_ = std::bind(&SimI2CListener::OnMessageFromAvr, this, std::placeholders::_1);

  avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_OUTPUT), FakeI2cCb2,
                          &OnMessageFromAvr_);

  OnMessageToAvr_ = std::bind(&SimI2CListener::OnMessageToAvr, this, std::placeholders::_1);

  avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_TWI_GETIRQ(0), TWI_IRQ_INPUT), FakeI2cCb2,
                          &OnMessageToAvr_);
}

void SimI2CListener::OnMessageFromAvr(avr_twi_msg_irq_t* value) {
  const avr_twi_msg_t msg = value->u.twi;

  if (msg.msg & TWI_COND_START) {
    if (MessageInProgress_) {
      Check(0, msg.addr >> 1);
      MessageInProgress_->RepeatedStart = true;
    } else {
      MessageInProgress_.emplace(Message(msg.addr >> 1));
    }
  } else if (msg.msg & TWI_COND_WRITE) {
    Check(1, msg.data);
    if (MessageInProgress_.has_value()) {
      MessageInProgress_->MsgType = MessageType::Write;
      MessageInProgress_->WriteBuffer.push_back(msg.data);
    }
  } else if (msg.msg & TWI_COND_READ) {
    Check(2, 0);
    if (MessageInProgress_.has_value()) {
      MessageInProgress_->MsgType = MessageType::Read;
    }
  } else if (msg.msg & TWI_COND_STOP) {
    if (MessageInProgress_.has_value()) {
      FinishedMessages_.push_front(*MessageInProgress_);
      while (FinishedMessages_.size() > 100) {
        FinishedMessages_.pop_back();
      }
      if (MessageCallbackFn_) {
        MessageCallbackFn_.value()(*MessageInProgress_);
      }
      MessageInProgress_ = std::nullopt;
    }
  }
}

void SimI2CListener::OnMessageToAvr(avr_twi_msg_irq_t* value) {
  const avr_twi_msg_t msg = value->u.twi;

  if (msg.msg & TWI_COND_READ) {
    Check(3, msg.data);
    MessageInProgress_->ReadBuffer.push_back(msg.data);
  }
}

void SimI2CListener::Check(int i, uint8_t data) {
  if (!MessageInProgress_.has_value()) {
    sim_debug_log("BAD I2C: %d ---> %d\n", i, (int)data);
  }
}

const SimI2CListener::FinishedMessages& SimI2CListener::GetFinishedMessages() const {
  return FinishedMessages_;
}

void SimI2CListener::OnMessage(MessageCallbackFn fn) {
  MessageCallbackFn_ = fn;
}
