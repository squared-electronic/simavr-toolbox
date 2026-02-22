#include "sim_base.hpp"

#include <cstdarg>

/*

TODO

template <class... Args>
void empty_log(std::format_string<Args...>, Args&&... args) {}

template <class... Args>
DebugLogFn<Args...> gDebugLogFn = empty_log;

template <class... Args>
void sim_debug_log(std::format_string<Args...>, Args&&... args) {
  gDebugLogFn<Args...>(args...);
}

template <class... Args>
void set_sim_debug_log(DebugLogFn<Args...> fn) {
  gDebugLogFn<Args...> = fn;
}

*/

void empty_log(const char*, va_list) {}

void (*gDebugLogFn)(const char*, va_list) = empty_log;

void sim_debug_log(const char* fmt, ...) {
  va_list myargs;
  va_start(myargs, fmt);
  gDebugLogFn(fmt, myargs);
  va_end(myargs);
}

void sim_debug_log(std::string_view s) {
  sim_debug_log(s.data());
}

void set_sim_debug_log(DebugLogFn fn) {
  gDebugLogFn = fn;
}