#pragma once

#include <condition_variable>
#include <mutex>

namespace cab {

class Semaphore {
 public:
  Semaphore(const Semaphore&) = delete;
  Semaphore(Semaphore&&) = delete;

  Semaphore& operator=(const Semaphore&) = delete;
  Semaphore& operator=(Semaphore&&) = delete;

  explicit Semaphore(int count = 0) {
    value_ = count;
  }

  void Post(int count = 1) {
    std::lock_guard<std::mutex> locker(mutex_);
    value_ += count;
    condition_.notify_all();
  }

  void Wait(int count = 1) {
    std::unique_lock<std::mutex> locker(mutex_);
    condition_.wait(locker, [this, count]() {
      return value_ >= count;
    });
    value_ -= count;
  }

  bool TryWait(int count = 1) {
    std::lock_guard<std::mutex> locker(mutex_);
    if (count > value_) {
      return false;
    }
    value_ -= count;
    return true;
  }

  int Value() const {
    std::lock_guard<std::mutex> locker(*const_cast<std::mutex*>(&mutex_));
    return value_;
  }

  void Clear() {
    std::lock_guard<std::mutex> locker(mutex_);
    value_ = 0;
  }

 private:
  int value_ = 0;
  std::mutex mutex_;
  std::condition_variable condition_;
};
}  // namespace cab