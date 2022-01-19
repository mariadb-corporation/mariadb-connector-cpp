// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (c) 2021 MariaDB Corporation Ab


#ifndef _BLOCKINGQUEUE_H_
#define _BLOCKINGQUEUE_H_

#include <queue>
#include <mutex>
#include <condition_variable>

namespace sql
{
  enum TimeUnit {
    NANOSECONDS,
    MICROSECONDS,
    SECONDS
  };

  class InterruptedException : public ::std::runtime_error
  {

  public:
    ~InterruptedException() {}

    InterruptedException& operator=(const InterruptedException&) = default;
    InterruptedException() = default;
    InterruptedException(const InterruptedException&) = default;
    InterruptedException(const char* msg) : std::runtime_error(msg) {}

    const char* getMessage() { return what(); }
    //char const* what() const noexcept override;
  };
  static const std::size_t DefaultQueueSize= 4;

  template <typename T> class blocking_deque {

    std::deque<T> realQueue;
    std::mutex queueSync;
    std::condition_variable notEmpty;
    bool closed= false;

  std::condition_variable notFull;
    std::size_t maxCapacity; // TODO: need to support it;

  public:
    using reverse_iterator= typename std::deque<T>::reverse_iterator;
    using const_iterator=   typename std::deque<T>::const_iterator;
    using iterator=         typename std::deque<T>::iterator;

    blocking_deque() : maxCapacity(DefaultQueueSize), realQueue()
    {}
    blocking_deque(std::size_t capacity) : maxCapacity(capacity),realQueue()
    {}

    void close()
    {
      bool notify= false;
      //LoggerFactory::getLogger().trace("Pool","Closing queue, notifying waiters");
      if (!closed)
      {
        std::unique_lock<std::mutex> lock(queueSync);
        if (!closed) {
          closed= true;
          notify= true;
        }
      }
      if (notify) {
        //LoggerFactory::getLogger().trace("Pool","Yes, do notifying waiters");
        notEmpty.notify_all();
      }
    }


    void push(T&& item)
    {
      {
        std::unique_lock<std::mutex> lock(queueSync);
        if (closed) {
          throw InterruptedException("The queue is closed");
        }
        realQueue.push_front(std::move(item));
      }
      notEmpty.notify_one();
    }


    void push(const T& item)
    {
      {
        std::unique_lock<std::mutex> lock(queueSync);
        if (closed) {
          throw InterruptedException("The queue is closed");
        }
        realQueue.push_front(item);
      }
      notEmpty.notify_one();
    }


    bool push_back(T&& item)
    {
      {
        std::unique_lock<std::mutex> lock(queueSync);
        if (closed) {
          return false;
        }
        realQueue.push_back(std::move(item));
      }
      notEmpty.notify_one();
      return true;
    }


    bool push_back(const T& item)
    {
      {
        std::unique_lock<std::mutex> lock(queueSync);
        if (closed) {
          return false;
        }
        realQueue.push_back(item);
      }
      notEmpty.notify_one();
      return true;
    }

    template <class... Args>
    bool emplace(Args&&... args)
    {
      {
        std::unique_lock<std::mutex> lock(queueSync);
        if (closed) {
          return false;
        }
        realQueue.emplace(realQueue.begin(), args...);
      }
      notEmpty.notify_one();
      return true;
    }

    template <class... Args>
    bool emplace_back(Args&&... args)
    {
      {
        std::unique_lock<std::mutex> lock(queueSync);
        if (closed) {
          return false;
        }
        realQueue.emplace_back(args...);
      }
      notEmpty.notify_one();
      return true;
    }

    /* Blocking call */
    /* TODO pop_back counterpart */
    void pop(T& item)
    {
      std::unique_lock<std::mutex> lock(queueSync);
      if (closed) {
        throw InterruptedException("The queue is closed");
      }
      while (!closed) {
        if (!realQueue.empty()) {
          break;
        }
        //LoggerFactory::getLogger().trace("Pool","<<< Nothing in the pipeline, waiting on condition");
        notEmpty.wait(lock);
        //LoggerFactory::getLogger().trace("Pool","<<< Condition met, proceeding");
      }
      // If waiters have been interrupted, the closed will be set and we should not return anything(we shouldn't have anything to)
      if (closed) {
        //LoggerFactory::getLogger().trace("Pool","<<< queue has been closed, throwing");
        throw InterruptedException("The queue is closed");
      }
      item= std::move(realQueue.front());
      realQueue.pop_front();
    }

    T pollFirst()
    {
      std::unique_lock<std::mutex> lock(queueSync);
      //for (;;)
      if (!closed && !realQueue.empty())
      {
        T result = std::move(realQueue.front());
        realQueue.pop_front();

        return std::move(result);
      }
      T null(nullptr);
      return std::move(null);
    }

    template<class Rep, class Period>
    T pollFirst(const std::chrono::duration<Rep, Period>& timeout_duration)
    {
      std::unique_lock<std::mutex> lock(queueSync);

      // If the queue is not closed yet, and is empty, let's wait for timeout duration
      if (!closed && realQueue.empty()) {
        notEmpty.wait_for(lock, timeout_duration);
      }

      T result(nullptr);
      // If still empty - then return "null", if not - return what we have
      if (!realQueue.empty()) {
        result= std::move(realQueue.front());
        realQueue.pop_front();
      }
      return std::move(result);
    }

    T pollFirst(int64_t timeout, enum TimeUnit unit)
    {
      switch (unit) {
      case SECONDS: return pollFirst(std::chrono::seconds(timeout));
      case MICROSECONDS: return pollFirst(std::chrono::microseconds(timeout));
      default/*case NANOSECONDS*/: return pollFirst(std::chrono::nanoseconds(timeout));
      }
    }

    /* that is application's responsibility to make sure it's not empty */
    T& front()
    {
      return realQueue.front();
    }

    T& back()
    {
      return realQueue.back();
    }

    reverse_iterator rbegin() { return realQueue.rbegin(); }
    reverse_iterator rend()   { return realQueue.rend(); }
    iterator begin() { return realQueue.begin(); }
    iterator end() { return realQueue.end(); }

    iterator erase(iterator it) {
      return realQueue.erase(it);
    }
    reverse_iterator erase(reverse_iterator it) {
      std::advance(it, 1);
      realQueue.erase(it.base());
      return it;
    }

    bool empty() { return realQueue.empty(); }
    std::size_t size() { return realQueue.size(); }
  };
}
#endif
