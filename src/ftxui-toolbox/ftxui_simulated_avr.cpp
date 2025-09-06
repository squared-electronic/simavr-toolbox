#include "ftxui_simulated_avr.hpp"

#include <simavr/avr_adc.h>
#include <simavr/avr_ioport.h>
#include <simavr/avr_twi.h>
#include <simavr/avr_uart.h>
#include <simavr/sim_avr.h>
#include <simavr/sim_elf.h>
#include <simavr/sim_gdb.h>
#include <simavr/sim_irq.h>

#include <cstring>
#include <ftxui/component/task.hpp>
#include <simavr-toolbox/sim_base.hpp>

void debug_log_uart0(struct avr_irq_t* irq, uint32_t value, void* param) {
  static std::string uartBuffer = "[Serial 0] ";
  uartBuffer += value;
  if (uartBuffer.back() == '\n') {
    sim_debug_log(uartBuffer.c_str());
    uartBuffer = "[Serial 0] ";
  }
}

void debug_log_uart1(struct avr_irq_t* irq, uint32_t value, void* param) {
  static std::string uartBuffer = "[Serial 1] ";
  uartBuffer += value;
  if (uartBuffer.back() == '\n') {
    sim_debug_log(uartBuffer.c_str());
    uartBuffer = "[Serial 1] ";
  }
}

FtxUiSimulatedAvr::FtxUiSimulatedAvr(std::string_view filename, bool gdb, TaskReceiver& receiver)
    : S_{receiver->MakeSender()} {
  Avr_ = LoadFirmware(filename, gdb);
}

avr_t* FtxUiSimulatedAvr::LoadFirmware(std::string_view filename, bool gdb) {
  avr_t* avr = nullptr;

  elf_firmware_t f;
  memset(&f, 0, sizeof(f));

  sim_debug_log("Firmware pathname is %s\n", filename.data());
  elf_read_firmware(filename.data(), &f);

  sim_debug_log("firmware %s f=%d mmcu=%s\n", filename.data(), (int)f.frequency, f.mmcu);

  avr = avr_make_mcu_by_name(f.mmcu);

  if (!avr) {
    sim_debug_log("AVR '%s' not known\n", f.mmcu);
    std::abort();
  }

  avr_init(avr);

  avr_load_firmware(avr, &f);

  // disable the stdio dump, as we have a TUI output
  uint32_t flags = 0;
  avr_ioctl(avr, AVR_IOCTL_UART_GET_FLAGS('0'), &flags);
  flags &= ~AVR_UART_FLAG_STDIO;
  avr_ioctl(avr, AVR_IOCTL_UART_SET_FLAGS('0'), &flags);

  // Debug log UART0
  avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUTPUT),
                          debug_log_uart0, nullptr);

  // Debug log UART1
  avr_irq_register_notify(avr_io_getirq(avr, AVR_IOCTL_UART_GETIRQ('1'), UART_IRQ_OUTPUT),
                          debug_log_uart1, nullptr);

  if (gdb) {
    avr->gdb_port = 1234;
    avr->state = cpu_Stopped;
    avr_gdb_init(avr);
  }

  return avr;
}

void FtxUiSimulatedAvr::BlockingLoop(std::atomic_bool& keepGoing,
                                     ftxui::Receiver<ftxui::Closure>& receiver) {
  auto state = Avr_->state;
  while ((state != cpu_Done) && (state != cpu_Crashed) && keepGoing) {
    ftxui::Closure t;
    while (receiver->ReceiveNonBlocking(&t)) {
      t();
    }
    BeforeAvrCycleSideEffect();
    state = avr_run(Avr_);
  }
}

void FtxUiSimulatedAvr::BeforeAvrCycleSideEffect() {
  //
}

avr_irq_t* FtxUiSimulatedAvr::GetPinIrq(char pin, uint8_t index) {
  return avr_io_getirq(Avr_, AVR_IOCTL_IOPORT_GETIRQ(pin), index);
}

void FtxUiSimulatedAvr::Post(ftxui::Closure&& f) {
  S_->Send(std::move(f));
}
