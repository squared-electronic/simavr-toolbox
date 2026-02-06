#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <simavr-toolbox/sim_i2c_base.hpp>
#include <vector>

class SimI2CListener {
 public:
  SimI2CListener(avr_t* avr);

  enum class MessageType {
    Read,
    Write,
  };

  struct Message {
    Message(uint8_t address) : Address{address} {}
    const uint8_t Address;
    std::optional<MessageType> Type;
    bool RepeatedStart{false};
    std::vector<uint8_t> WriteBuffer;
    std::vector<uint8_t> ReadBuffer;
    auto operator<=>(const Message&) const = default;
  };

  using FinishedMessages = std::deque<Message>;
  using MessageCallbackFn = std::function<void(const Message&)>;

  const FinishedMessages& GetFinishedMessages() const;
  void OnMessage(MessageCallbackFn fn);

 private:
  void OnMessageFromAvr(const avr_twi_msg_irq_t& value);
  void OnMessageToAvr(const avr_twi_msg_irq_t& value);
  void Check(int i, uint8_t data);

  FinishedMessages FinishedMessages_;
  std::optional<Message> MessageInProgress_;
  std::optional<MessageCallbackFn> MessageCallbackFn_;
};
