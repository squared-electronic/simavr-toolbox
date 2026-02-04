#include <cstdint>
#include <string>
#include <vector>

struct UartLineBufferLogger {
  UartLineBufferLogger(int serialNumber);
  void OnSerialByte(uint32_t value);
  virtual void OnSerialLine(const std::string& s) = 0;

 private:
  const int Number_;
  std::vector<uint8_t> LineBuffer_;
};
