#include "utils/ThreadPool.h"

ThreadPool::ThreadPool(size_t numThreads) : stop_(false) {
  for (size_t i = 0; i < numThreads; i++) {
    workers_.emplace_back([this]() {
      while (true) {
        std::function<void()> task;

        {
          std::unique_lock<std::mutex> lock(queueMutex_);
          condition_.wait(lock, [this]() {
            return stop_ || !tasks_.empty();
          });

          if (stop_ && tasks_.empty()) {
            return;
          }

          task = std::move(tasks_.front());
          tasks_.pop();
        }

        task();
      }
    });
  }
}

ThreadPool::~ThreadPool() {
  {
    std::unique_lock<std::mutex> lock(queueMutex_);
    stop_ = true;
  }

  condition_.notify_all();

  for (auto& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void ThreadPool::enqueue(std::function<void()> task) {
  {
    std::unique_lock<std::mutex> lock(queueMutex_);
    if (stop_) {
      return;
    }
    tasks_.push(std::move(task));
  }
  condition_.notify_one();
}

size_t ThreadPool::pendingTasks() const {
  std::unique_lock<std::mutex> lock(queueMutex_);
  return tasks_.size();
}