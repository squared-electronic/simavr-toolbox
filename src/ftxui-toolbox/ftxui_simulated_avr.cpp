#include "ftxui_simulated_avr.hpp"

#include <simavr/avr_adc.h>
#include <simavr/avr_ioport.h>
#include <simavr/avr_twi.h>
#include <simavr/avr_uart.h>
#include <simavr/sim_avr.h>
#include <simavr/sim_elf.h>
#include <simavr/sim_gdb.h>
#include <simavr/sim_irq.h>

#include <cstdint>
#include <cstring>
#include <ftxui/component/task.hpp>
#include <simavr-toolbox/sim_base.hpp>

FtxUiSimulatedAvr::FtxUiSimulatedAvr(std::string_view filename, bool gdb, TaskReceiver& receiver)
    : S_{receiver->MakeSender()} {
  Avr_ = LoadFirmware(filename, gdb);

  // disable the stdio dump, as we have a byte handler
  uint32_t flags = 0;
  avr_ioctl(Avr_, AVR_IOCTL_UART_GET_FLAGS('0'), &flags);
  flags &= ~AVR_UART_FLAG_STDIO;
  avr_ioctl(Avr_, AVR_IOCTL_UART_SET_FLAGS('0'), &flags);

  avr_ioctl(Avr_, AVR_IOCTL_UART_GET_FLAGS('1'), &flags);
  flags &= ~AVR_UART_FLAG_STDIO;
  avr_ioctl(Avr_, AVR_IOCTL_UART_SET_FLAGS('1'), &flags);

  auto receiver0 = [](struct avr_irq_t* irq, uint32_t value, void* param) {
    auto that = (FtxUiSimulatedAvr*)param;
    that->OnUartByteReceived(0, value);
  };

  auto receiver1 = [](struct avr_irq_t* irq, uint32_t value, void* param) {
    auto that = (FtxUiSimulatedAvr*)param;
    that->OnUartByteReceived(1, value);
  };

  // Debug log UART0
  avr_irq_register_notify(avr_io_getirq(Avr_, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUTPUT),
                          receiver0, this);

  // Debug log UART1
  avr_irq_register_notify(avr_io_getirq(Avr_, AVR_IOCTL_UART_GETIRQ('1'), UART_IRQ_OUTPUT),
                          receiver1, this);
}

avr_t* FtxUiSimulatedAvr::LoadFirmware(std::string_view filename, bool gdb) {
  avr_t* avr = nullptr;

  elf_firmware_t f;
  memset(&f, 0, sizeof(f));

  elf_read_firmware(filename.data(), &f);

  sim_debug_log("f=%d mmcu=%s\n", (int)f.frequency, f.mmcu);

  avr = avr_make_mcu_by_name(f.mmcu);

  if (!avr) {
    sim_debug_log("AVR '%s' not known\n", f.mmcu);
    std::abort();
  }

  avr_init(avr);

  avr_load_firmware(avr, &f);

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

void FtxUiSimulatedAvr::OnUartByteReceived(int, uint8_t) {
  //
}

avr_irq_t* FtxUiSimulatedAvr::GetPinIrq(char pin, uint8_t index) {
  return avr_io_getirq(Avr_, AVR_IOCTL_IOPORT_GETIRQ(pin), index);
}

void FtxUiSimulatedAvr::Post(ftxui::Closure&& f) {
  S_->Send(std::move(f));
}
