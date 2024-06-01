

#pragma once

#include <functional>

#include "sim_avr_types.h"

using IrqCallback = std::function<avr_cycle_count_t(avr_cycle_count_t when)>;

void RegisterTimer(avr_t* avr, IrqCallback cb, avr_cycle_count_t when);