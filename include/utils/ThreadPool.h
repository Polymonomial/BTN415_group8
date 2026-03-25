#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// Simple thread pool for handling concurrent client connections
class ThreadPool {
 public:
  explicit ThreadPool(size_t numThreads);
  ~ThreadPool();

  // Submit a task to the pool
  void enqueue(std::function<void()> task);

  // Get the number of pending tasks
  size_t pendingTasks() const;

 private:
  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;

  mutable std::mutex queueMutex_;
  std::condition_variable condition_;
  bool stop_;
};

#endif  // THREAD_POOL_H