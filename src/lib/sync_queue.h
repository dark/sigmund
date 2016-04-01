/*
 *  SIGnatures Monitor and UNifier Daemon
 *  Copyright (C) 2016  Marco Leogrande
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

namespace freud {
namespace lib {

template <typename T>
class SyncQueue {
 public:
  SyncQueue() = default;
  ~SyncQueue() = default;

  void push(T *datum) {
    {
      // push datum in the queue under the lock
      std::lock_guard<std::mutex> lock_guard(mutex_);
      queue_.push(datum);
    }

    // notify all waiting threads
    cv_.notify_all();
  }

  T* pop_or_wait() {
    // pop datum from queue under a lock
    std::unique_lock<std::mutex> lock(mutex_);

    // wait until awoken if no datum is readily available
    while (queue_.empty())
      // in theory, the lambda should be enough to cover spurious
      // wakeups, but use a wrapping while loop just in case
      cv_.wait(lock, [this]{return !queue_.empty();});

    T *datum = queue_.front();
    queue_.pop();
    return datum;
  }

  T* nonblocking_pop() {
    // pop datum from queue under a lock
    std::lock_guard<std::mutex> lock_guard(mutex_);
    if (queue_.empty())
      // nothing to pop
      return NULL;

    T *datum = queue_.front();
    queue_.pop();
    return datum;
  }

 private:
  std::queue<T*> queue_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

} // namespace lib
} // namespace freud
