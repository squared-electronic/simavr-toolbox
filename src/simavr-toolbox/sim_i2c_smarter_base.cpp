#include "sim_i2c_smarter_base.hpp"

void SimAvrI2CSmarterComponent::HandleI2CMessage(const avr_twi_msg_t& msg) {
  if (msg.msg & TWI_COND_START) {
    SendToAvrI2CAck();
  } else if (msg.msg & TWI_COND_STOP) {
    OnDataReceived(DataBuffer_);
    SendToAvrI2CAck();
    DataBuffer_.clear();
  } else if (msg.msg & TWI_COND_WRITE) {
    DataBuffer_.push_back(msg.data);
    SendToAvrI2CAck();
  } else if (msg.msg & TWI_COND_READ) {
    // No ack
  }
}
