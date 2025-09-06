#pragma once
#pragma once

#include <simavr/sim_avr.h>

#include <ftxui/component/receiver.hpp>
#include <ftxui/component/task.hpp>

using TaskSender = ftxui::Sender<ftxui::Closure>;
using TaskReceiver = ftxui::Receiver<ftxui::Closure>;

class FtxUiSimulatedAvr {
 public:
  FtxUiSimulatedAvr(std::string_view filename, bool gdb,
                    TaskReceiver& receiver);
  void BlockingLoop(std::atomic_bool&, TaskReceiver& receiver);
  static avr_t* LoadFirmware(std::string_view filename, bool gdb);

 protected:
  void Post(ftxui::Closure&& f);

  TaskSender S_;
  virtual void BeforeAvrCycleSideEffect();
  avr_irq_t* GetPinIrq(char pin, uint8_t index);
  avr_t* Avr_{nullptr};
};