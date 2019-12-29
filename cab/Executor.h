#pragma once

#include <functional>
#include <thread>
#include <vector>

#include "BlockingQueue.h"

namespace cab {

class Executor {
 public:
  using Job = std::function<void(void)>;

 public:
  Executor() = default;
  Executor(const Executor&) = delete;
  Executor(Executor&&) = delete;
  auto operator=(const Executor&) -> Executor& = delete;
  auto operator=(Executor &&) -> Executor& = delete;

  ~Executor() {
    Stop();
  }

  void Start(unsigned int n = 0) {
    if (n == 0) {
      n = std::thread::hardware_concurrency();
    }
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
    for (auto& thread : threads_) {
      thread.join();
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

  [[nodiscard]] auto Size() const -> size_t {
    return queue_.Size();
  }

  [[nodiscard]] auto Empty() const -> bool {
    return queue_.Empty();
  }

 private:
  BlockingQueue<Job> queue_;
  std::vector<std::thread> threads_;
};

}  // namespace cab
