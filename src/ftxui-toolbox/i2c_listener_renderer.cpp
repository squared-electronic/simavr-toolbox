#include "i2c_listener_renderer.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <format>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>
#include <simavr-toolbox/sim_i2c_listener.hpp>
#include <string>
#include <unordered_map>
#include <utility>

static const std::array<ftxui::Color, 5> Colors = {
    ftxui::Color::Cyan,   ftxui::Color::Red,       ftxui::Color::Green1,
    ftxui::Color::Yellow, ftxui::Color::BlueLight,
};

ftxui::Color RandomColor() {
  return Colors[rand() % Colors.size()];
}

class I2CListenerRendererBase : public ftxui::ComponentBase {
 public:
  I2CListenerRendererBase(avr_t* avr) : i2c_listener_(avr) {
    // Create a simple scroller with all I2C messages
    Add(ftxui::Renderer([&] { return RenderI2CListener(); }));
    i2c_listener_.OnMessage([this](const auto& m) { OnI2cMessage(m); });
  }

 private:
  ftxui::Element RenderI2CListener() {
    // If no messages yet, show a placeholder
    if (ReceivedMessages_.empty()) {
      return ftxui::vbox({ftxui::text("I2C Listener") | ftxui::bold, ftxui::separator(),
                          ftxui::text("No I2C messages received yet")});
    }

    // Create a list of all messages
    std::vector<ftxui::Element> message_elements;

    for (const auto& [msg, count] : ReceivedMessages_) {
      std::string address_str = std::format("0x{:02X}", msg.Address);
      std::string message_str = RenderMessage(msg, count);

      if (!AddressColorMap_.contains(msg.Address)) {
        AddressColorMap_[msg.Address] = RandomColor();
      }

      message_elements.push_back(ftxui::hbox(
          {ftxui::text(address_str) | ftxui::color(AddressColorMap_.at(msg.Address)) | ftxui::bold,
           ftxui::text(": "), ftxui::text(message_str) | ftxui::color(ftxui::Color::White)}));
    }

    return ftxui::vbox({ftxui::text("I2C Listener") | ftxui::bold, ftxui::separator(),
                        ftxui::vbox(message_elements) | ftxui::vscroll_indicator | ftxui::frame});
  }

 private:
  typedef std::pair<SimI2CListener::Message, uint32_t> MessageWithCount;

  void OnI2cMessage(const SimI2CListener::Message& m) {
    if (!ReceivedMessages_.empty() && ReceivedMessages_.front().first == m) {
      ReceivedMessages_.front().second += 1;
    } else {
      ReceivedMessages_.emplace_front(m, 0);
    }
  }

  static std::string RenderMessage(const SimI2CListener::Message& message, uint32_t count) {
    auto x =
        std::format("R/W: {} RS: {}",
                    message.MsgType.has_value()
                        ? message.MsgType.value() == SimI2CListener::MessageType::Read ? "R" : "W"
                        : "?",
                    message.RepeatedStart ? "Y" : "N");

    if (!message.WriteBuffer.empty()) {
      x += std::format(" WD: ");
      for (const auto& data : message.WriteBuffer) {
        auto y = std::format("0x{:0X} ", data);
        x += y;
      }
    }

    if (!message.ReadBuffer.empty()) {
      x += std::format(" RD: ");
      for (const auto& data : message.ReadBuffer) {
        auto y = std::format("0x{:0X} ", data);
        x += y;
      }
    }

    if (count > 1) {
      x += std::format(" [{}]", count);
    }

    return x;
  }

  SimI2CListener i2c_listener_;
  std::deque<MessageWithCount> ReceivedMessages_;
  std::unordered_map<uint8_t /* Address  */, ftxui::Color> AddressColorMap_;
};

ftxui::Component I2CListenerRenderer(avr_t* avr) {
  return ftxui::Make<I2CListenerRendererBase>(avr);
}