#include "timer.hpp"

#include "sim_cycle_timers.h"

avr_cycle_count_t FakeTimerCb(struct avr_t *avr, avr_cycle_count_t when, void *param) {
  auto x = (IrqCallback *)param;
  return (*x)(when);
}

void RegisterTimer(avr_t *avr, IrqCallback cb, avr_cycle_count_t when) {
  avr_cycle_timer_register(avr, when, &FakeTimerCb, &cb);
}