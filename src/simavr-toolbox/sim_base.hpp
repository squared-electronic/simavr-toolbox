#pragma once

#include <stdarg.h>

#include <string_view>

using DebugLogFn = void (*)(const char* fmt, va_list);

void sim_debug_log(const char* fmt, ...);

void sim_debug_log(std::string_view s);

void set_sim_debug_log(DebugLogFn fn);