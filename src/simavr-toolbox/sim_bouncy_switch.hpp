#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <queue>

#include "sim_avr.h"
#include "sim_avr_types.h"
#include "sim_irq.h"

class SimBouncySwitch {
 public:
  SimBouncySwitch(avr_t& avr, avr_irq_t& pin, bool closedValue);

  void CloseForMs(std::chrono::milliseconds ms);
  void OpenForMs(std::chrono::milliseconds ms);
  void Close();
  void Open();
  void Set(bool value);

 private:
  static constexpr auto ZeroMs = std::chrono::milliseconds(0);
  using TimerCb = std::function<avr_cycle_count_t(avr_cycle_count_t)>;

  struct LevelShift {
    LevelShift(bool targetValue, std::chrono::milliseconds holdTimeMs);
    uint8_t BounceCount_;
    bool TargetValue_;
    std::chrono::microseconds HoldTimeUs_;
  };

  static int RandInt(int low, int high);
  static avr_cycle_count_t FakeCb(struct avr_t* avr, avr_cycle_count_t when, void* param);
  void RestartBounces();
  avr_cycle_count_t OnBounce(avr_cycle_count_t cyclesNow);
  std::queue<LevelShift> PendingShifts_;
  bool BouncingValue_;
  avr_t& Avr_;
  avr_irq_t& Pin_;
  TimerCb BounceCb_;
  const bool ClosedValue_;
};
