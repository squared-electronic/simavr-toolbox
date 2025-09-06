#pragma once

#include <deque>
#include <functional>
#include <mutex>
#include <string>

struct LockedDequeue {
  void Insert(const std::string& s) {
    M_.lock();
    Logs_.push_front(std::move(s));
    if (Logs_.size() >= 250) {
      Logs_.pop_back();
    }
    M_.unlock();
  }

  void ForEach(std::function<void(const std::string&)> f) const {
    M_.lock();
    for (const auto& x : Logs_) {
      f(x);
    }
    M_.unlock();
  }

 private:
  mutable std::mutex M_;
  std::deque<std::string> Logs_;
};
