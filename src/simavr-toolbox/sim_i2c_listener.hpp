#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <simavr-toolbox/sim_i2c_base.hpp>
#include <vector>

using I2cStructMessageCallback = std::function<void(avr_twi_msg_irq_t*)>;

class SimI2CListener {
 public:
  enum class MessageType {
    Read,
    Write,
  };

  struct Message {
    Message(uint8_t address) : Address{address} {}
    const uint8_t Address;
    std::optional<MessageType> MsgType;
    bool RepeatedStart{false};
    std::vector<uint8_t> WriteBuffer;
    std::vector<uint8_t> ReadBuffer;
    auto operator<=>(const Message&) const = default;
  };

  using FinishedMessages = std::deque<Message>;
  using MessageCallbackFn = std::function<void(const Message&)>;

  SimI2CListener(avr_t* avr);
  const FinishedMessages& GetFinishedMessages() const;
  void OnMessage(MessageCallbackFn fn);

 private:
  void OnMessageFromAvr(avr_twi_msg_irq_t* value);
  void OnMessageToAvr(avr_twi_msg_irq_t* value);

  I2cStructMessageCallback OnMessageFromAvr_;
  I2cStructMessageCallback OnMessageToAvr_;
  FinishedMessages FinishedMessages_;
  std::optional<Message> MessageInProgress_;
  std::optional<MessageCallbackFn> MessageCallbackFn_;
};