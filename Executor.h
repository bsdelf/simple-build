#pragma once

#include <functional>
#include <thread>
#include <vector>

#include "BlockingQueue.h"

class Executor {
 public:
  using Job = std::function<void(void)>;
  using JobQueue = ccbb::BlockingQueue<Job>;

 public:
  ~Executor() {
    Stop();
  }

  void Start() {
    const auto n = std::thread::hardware_concurrency();
    Start(n);
  }

  void Start(unsigned int n) {
    for (auto i = 0u; i < n; ++i) {
      threads_.emplace_back([this]() {
        while (true) {
          auto job = queue_.Take();
          if (!job) {
            return;
          }
          job();
        }
      });
    }
  }

  void Stop(bool now = false) {
    for (size_t i = 0; i < threads_.size(); ++i) {
      if (now) {
        queue_.EmplaceFront(nullptr);
      } else {
        queue_.EmplaceBack(nullptr);
      }
    }
    for (size_t i = 0; i < threads_.size(); ++i) {
      threads_[i].join();
    }
    threads_.clear();
  }

  void Push(Job job) {
    if (job) {
      queue_.PushBack(std::move(job));
    }
  }

  void Clear() {
    queue_.Clear();
  }

  size_t Size() const {
    return queue_.Size();
  }

  bool Empty() const {
    return queue_.Empty();
  }

 private:
  JobQueue queue_;
  std::vector<std::thread> threads_;
};