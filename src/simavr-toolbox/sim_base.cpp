#include "sim_base.hpp"

#include <cstdarg>

void empty_log(const char*, va_list) {}

void (*gDebugLogFn)(const char*, va_list) = empty_log;

void sim_debug_log(const char* fmt, ...) {
  va_list myargs;
  va_start(myargs, fmt);
  gDebugLogFn(fmt, myargs);
  va_end(myargs);
}

void set_sim_debug_log(DebugLogFn fn) {
  gDebugLogFn = fn;
}