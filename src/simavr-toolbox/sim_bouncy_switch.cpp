#include "sim_bouncy_switch.hpp"

#include <cstdlib>

#include "sim_cycle_timers.h"
#include "sim_time.h"

// =========================================================================

int SimBouncySwitch::RandInt(int low, int high) {
  return low + rand() % (high - low);
}

avr_cycle_count_t SimBouncySwitch::FakeCb(struct avr_t* avr, avr_cycle_count_t when, void* param) {
  auto cb = (TimerCb*)param;
  return (*cb)(when);
}

SimBouncySwitch::SimBouncySwitch(avr_t& avr, avr_irq_t& pin, bool closedValue)
    : Avr_(avr), Pin_(pin), ClosedValue_(closedValue) {
  BounceCb_ = std::bind(&SimBouncySwitch::OnBounce, this, std::placeholders::_1);
}

void SimBouncySwitch::CloseForMs(std::chrono::milliseconds ms) {
  PendingShifts_.emplace(ClosedValue_, ms);
  PendingShifts_.emplace(!ClosedValue_, ZeroMs);
  RestartBounces();
}

void SimBouncySwitch::OpenForMs(std::chrono::milliseconds ms) {
  PendingShifts_.emplace(!ClosedValue_, ms);
  PendingShifts_.emplace(ClosedValue_, ZeroMs);
  RestartBounces();
}

void SimBouncySwitch::Close() {
  PendingShifts_.emplace(ClosedValue_, ZeroMs);
  RestartBounces();
}

void SimBouncySwitch::Open() {
  PendingShifts_.emplace(ClosedValue_, ZeroMs);
  RestartBounces();
}

void SimBouncySwitch::Set(bool value) {
  PendingShifts_.emplace(value, ZeroMs);
  RestartBounces();
}

void SimBouncySwitch::RestartBounces() {
  auto timeUntilFirstBounceUsec = RandInt(0, 1000);
  avr_cycle_timer_register_usec(&Avr_, timeUntilFirstBounceUsec, &FakeCb, &BounceCb_);
}

avr_cycle_count_t SimBouncySwitch::OnBounce(avr_cycle_count_t cyclesNow) {
  // If we are done bouncing for this change, and should
  // se the signal to the final value
  if (auto& front = PendingShifts_.front(); front.BounceCount_ == 0) {
    avr_raise_irq(&Pin_, front.TargetValue_);
    // If we have another shift scheduled, set to bounce again after the hold time of the
    // bounce we just finished
    auto holdTimeNow = front.HoldTimeUs_;
    PendingShifts_.pop();
    if (PendingShifts_.size()) {
      auto nextBounceTime = cyclesNow + avr_usec_to_cycles(&Avr_, holdTimeNow.count());
      return nextBounceTime;
    } else {
      // Completed all bounces; stop this timer chain
      return 0;
    }
  } else {
    // Bounce ths signal. More bounces to go for this level shift
    PendingShifts_.front().BounceCount_ -= 1;
    if (RandInt(0, 10) >= 6) {
      BouncingValue_ = !BouncingValue_;
    }
    avr_raise_irq(&Pin_, BouncingValue_);
    auto nextBounceTime = cyclesNow + avr_usec_to_cycles(&Avr_, RandInt(0, 1000));
    return nextBounceTime;
  }
}

// =========================================================================

SimBouncySwitch::LevelShift::LevelShift(bool targetValue, std::chrono::milliseconds holdTimeMs)
    : BounceCount_{10}, TargetValue_{targetValue}, HoldTimeUs_{holdTimeMs} {}
