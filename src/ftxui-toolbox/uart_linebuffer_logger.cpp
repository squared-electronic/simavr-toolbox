#include "uart_linebuffer_logger.hpp"

#include <format>
#include <string>

UartLineBufferLogger::UartLineBufferLogger(int serialNumber) : Number_{serialNumber} {};

void UartLineBufferLogger::OnSerialByte(uint32_t value) {
  LineBuffer_.push_back(value);
  if (value == '\n') {
    std::string s = std::format("[Serial {}] ", Number_);
    for (uint8_t byte : LineBuffer_) {
      if (std::isprint(byte) || std::isspace(byte)) {
        s += (char)byte;
      } else {
        s += std::format("0x{:0X} ", byte);
      }
    }
    LineBuffer_.clear();
    OnSerialLine(s);
  }
}
