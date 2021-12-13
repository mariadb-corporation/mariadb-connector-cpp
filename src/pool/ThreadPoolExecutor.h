/*
 *
 * MariaDB  C++ Connector
 *
 * Copyright (c) 2021 MariaDB Ab.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not see <http://www.gnu.org/licenses>
 * or write to the Free Software Foundation, Inc., 
 * 51 Franklin St., Fifth Floor, Boston, MA 02110, USA
 *
 */

#ifndef _THREADPOOLEXECUTOR_H_
#define _THREADPOOLEXECUTOR_H_ 

#include <functional>
#include <atomic>
#include <chrono>
#include "compat/Executor.hpp"
#include "util/BlockingQueue.h"
#include "MariaDbThreadFactory.h"

/* It shouldn't be in sql, probably. And if in sql, then maybe sql::mariadb is a better choice,
 * but it also can happen, that we will need to expose something in the public interface. Thus
 * leaving it in the sql so far
 */
namespace sql
{

class Runnable{
  std::function<void()> codeToRun;

public:
  Runnable(const Runnable&)= default;
  Runnable& operator=(const Runnable &other);
  Runnable() : codeToRun() {}
  Runnable(std::function<void()> func) : codeToRun(func) {}
  Runnable(Runnable&& moved);
  virtual ~Runnable(){}

  virtual void run() { codeToRun(); }/*=0*/;
};


/*Stub to enable compilation */
class ScheduledFuture
{
  ScheduledFuture()= delete;
  std::atomic<bool>& workersQuitFlag;
public:
  ScheduledFuture(std::atomic<bool>& flagRef);
  void cancel(bool cancelType);
};


struct ScheduledTask
{
  std::chrono::seconds schedulePeriod;
  std::chrono::time_point<std::chrono::steady_clock> nextRunTime;
  std::unique_ptr<std::atomic_bool> canceled;
  Runnable task;

  ScheduledTask(Runnable taskCode, uint32_t seconds = 0) : 
    task(taskCode), schedulePeriod(seconds),
    nextRunTime(std::chrono::steady_clock::now() + schedulePeriod),
    canceled(new std::atomic_bool(false)) {}

  bool operator()();
};

class ThreadPoolExecutor: public Executor {
  ThreadPoolExecutor(const ThreadPoolExecutor&);
  void operator=(ThreadPoolExecutor&);
  ThreadPoolExecutor()=delete;

protected:
  blocking_deque<Runnable> localQueue;
  blocking_deque<Runnable>& tasksQueue;
  std::unique_ptr<ThreadFactory> threadFactory;
  int32_t corePoolSize;
  int32_t maximumPoolSize;
  bool allowTimeout;
  std::atomic_int workersCount;
  std::vector<std::thread> workersList;
  std::atomic_bool quit;

  Runnable worker;
  virtual void workerFunction();

public:
  virtual ~ThreadPoolExecutor();

  ThreadPoolExecutor(int32_t corePoolSize, int32_t maximumPoolSize, int64_t keepAliveTime, TimeUnit unit,
    blocking_deque<Runnable>& workQueue, ThreadFactory* _threadFactory);
  ThreadPoolExecutor(int32_t corePoolSize, int32_t maximumPoolSize, ThreadFactory* _threadFactory)
    : ThreadPoolExecutor(corePoolSize, maximumPoolSize, 0, TimeUnit::SECONDS, localQueue, _threadFactory)
  {}
  void allowCoreThreadTimeOut(bool value);
  virtual bool prestartCoreThread();
  virtual void shutdown();
  template <class T, class P>
  bool awaitTermination(std::chrono::duration<T,P> period);
  void execute(Runnable code);
  // it's defined this way in the (public) interface and needs to be implemented
  void execute(std::function<void()> func);
};

// -------------------------------- ScheduledThreadPoolExecutor ----------------------------------

class ScheduledThreadPoolExecutor : public /*ThreadPool*/Executor
{
  ScheduledThreadPoolExecutor(const ScheduledThreadPoolExecutor&);
  void operator=(ScheduledThreadPoolExecutor&);
  ScheduledThreadPoolExecutor() = delete;

  std::unique_ptr<ThreadFactory> threadFactory;
  blocking_deque<ScheduledTask*> tasksQueue;
  std::atomic_int workersCount;
  std::atomic_bool quit;
  std::vector<std::thread> workersList;
  int32_t corePoolSize;
  int32_t maximumPoolSize;
  Runnable worker;

  void workerFunction();
public:
  virtual ~ScheduledThreadPoolExecutor();
  ScheduledThreadPoolExecutor(int32_t corePoolSize, int32_t maximumPoolSize, ThreadFactory* _threadFactory);
  ScheduledThreadPoolExecutor(int32_t coreSize, ThreadFactory* thf) : ScheduledThreadPoolExecutor(coreSize, coreSize, thf) {}
  ScheduledFuture* scheduleAtFixedRate(std::function<void(void)> methodToRun, int32_t scheduleDelay, int32_t delay2,
    enum TimeUnit unit);

  bool prestartCoreThread();
  virtual void shutdown();
  template <class T, class P>
  bool awaitTermination(std::chrono::duration<T, P> period);
  void execute(std::function<void()>);
  void execute(Runnable code);
  //bool isShutdown()
};
}
#endif
