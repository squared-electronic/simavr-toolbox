#pragma once

#include <stdarg.h>

using DebugLogFn = void (*)(const char* fmt, va_list);

void sim_debug_log(const char* fmt, ...);

void set_sim_debug_log(DebugLogFn fn);