#pragma once

#ifdef USE_POSIX_SEMAPHORE  // POSIX unnamed semaphore
#include <semaphore.h>
#else
#include <condition_variable>
#include <mutex>
#endif

namespace cab {

class Semaphore {
 public:
  Semaphore(const Semaphore&) = delete;
  Semaphore& operator=(const Semaphore&) = delete;

  explicit Semaphore(int value = 0) {
#ifdef USE_POSIX_SEMAPHORE
    ::sem_init(&m_sem, 0, value);
#else
    value_ = value;
#endif
  }

#ifdef USE_POSIX_SEMAPHORE
  Semaphore(int pshared, int value) {
    ::sem_init(&m_sem, pshared, value);
  }
#endif

  ~Semaphore() {
#ifdef USE_POSIX_SEMAPHORE
    ::sem_destroy(&m_sem);
#endif
  }

  void Post(int n = 1) {
#ifdef USE_POSIX_SEMAPHORE
    while (n-- > 0)
      ::sem_post(&m_sem);
#else
    std::lock_guard<std::mutex> locker(mutex_);
    value_ += n;
    condition_.notify_all();
#endif
  }

  bool TryWait(int n = 1) {
#ifdef USE_POSIX_SEMAPHORE
    while (n > 0 && ::sem_trywait(&m_sem) == 0)
      --n;
    return (n == 0);
#else
    std::lock_guard<std::mutex> locker(mutex_);
    if (n > value_)
      return false;
    value_ -= n;
    return true;
#endif
  }

  void Wait(int n = 1) {
#ifdef USE_POSIX_SEMAPHORE
    while (n-- > 0)
      ::sem_wait(&m_sem);
#else
    std::unique_lock<std::mutex> locker(mutex_);
    while (n > value_)
      condition_.wait(locker);
    value_ -= n;
#endif
  }

  int Value() const {
#ifdef USE_POSIX_SEMAPHORE
    int sval = 0;
    ::sem_getvalue(&m_sem, &sval);
    return sval;
#else
    std::lock_guard<std::mutex> locker(*const_cast<std::mutex*>(&mutex_));
    return value_;
#endif
  }

  void Clear() {
#ifdef USE_POSIX_SEMAPHORE
    while (::sem_trywait(&m_sem) == 0)
      ;
#else
    std::lock_guard<std::mutex> locker(mutex_);
    value_ = 0;
#endif
  }

 private:
#ifdef USE_POSIX_SEMAPHORE
  mutable sem_t m_sem;
#else
  int value_ = 0;
  std::mutex mutex_;
  std::condition_variable condition_;
#endif
};
}  // namespace cab