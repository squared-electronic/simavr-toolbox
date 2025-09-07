#include "i2c_listener_renderer.hpp"

#include <format>
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <simavr-toolbox/sim_i2c_listener.hpp>

class I2CListenerRendererBase : public ftxui::ComponentBase {
 public:
  I2CListenerRendererBase(avr_t* avr) : i2c_listener_(avr) {
    // Create a simple scroller with all I2C messages
    Add(ftxui::Renderer([&] { return RenderI2CListener(); }));
  }

 private:
  ftxui::Element RenderI2CListener() {
    const auto& messages = i2c_listener_.GetFinishedMessages();

    // If no messages yet, show a placeholder
    if (messages.empty()) {
      return ftxui::vbox({ftxui::text("I2C Listener") | ftxui::bold, ftxui::separator(),
                          ftxui::text("No I2C messages received yet")});
    }

    // Create a list of all messages
    std::vector<ftxui::Element> message_elements;

    for (const auto& msg : messages) {
      std::string address_str = std::format("0x{:02X}", msg.Address);
      std::string message_str = RenderMessage(msg);

      message_elements.push_back(ftxui::hbox(
          {ftxui::text(address_str) | ftxui::color(ftxui::Color::Cyan) | ftxui::bold,
           ftxui::text(": "), ftxui::text(message_str) | ftxui::color(ftxui::Color::White)}));
    }

    return ftxui::vbox({ftxui::text("I2C Listener") | ftxui::bold, ftxui::separator(),
                        ftxui::vbox(message_elements) | ftxui::vscroll_indicator | ftxui::frame});
  }

 private:
  static std::string RenderMessage(const SimI2CListener::Message& message) {
    auto x =
        std::format("R/W: {} RS: {}",
                    message.MsgType.has_value()
                        ? message.MsgType.value() == SimI2CListener::MessageType::Read ? "R" : "W"
                        : "?",
                    message.RepeatedStart);

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

    return x;
  }

  SimI2CListener i2c_listener_;
};

ftxui::Component I2CListenerRenderer(avr_t* avr) {
  return ftxui::Make<I2CListenerRendererBase>(avr);
}