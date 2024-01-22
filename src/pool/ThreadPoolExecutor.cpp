// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (c) 2021 MariaDB Corporation Ab

#include "ThreadPoolExecutor.h"
#include "MariaDbThreadFactory.h"
#include <iostream>

namespace sql
{
  template <class T, class P>
  static bool awaitTermination(std::chrono::duration<T, P> waitTime, std::atomic_int& workersCount, std::vector<std::thread>& workers)
  {
    auto endPoint = std::chrono::steady_clock::now() + waitTime;
    auto defaultSleep = std::chrono::milliseconds(5);
    auto sleepPeriod = waitTime / 100;

    /* We do not want to check too often */
    if (sleepPeriod < defaultSleep && defaultSleep < waitTime) {
      sleepPeriod = std::chrono::duration_cast<std::chrono::duration<T, P>>(defaultSleep);
    }
    //while (std::chrono::steady_clock::now() < endPoint && workersCount.load() != 0)
    while (std::chrono::steady_clock::now() < endPoint) {
      int32_t count = workersCount.load();
      //LoggerFactory::getLogger().trace("Pool", count, " : ", endPoint - std::chrono::steady_clock::now()).count());
      if (count == 0) {
        return true;
      }
      std::this_thread::sleep_for(sleepPeriod);
    }
    return workersCount.load() == 0;
  }

//-------------------- ThreadPoolExecutor ---------------------

void ThreadPoolExecutor::workerFunction()
{
  Runnable task;
  //LoggerFactory::getLogger().trace("Pool", "Starting appender thread");
  while (!quit.load())
  {
    //LoggerFactory::getLogger().trace("Pool", "Quit not set, requesting task");
    try {
      tasksQueue.pop(task);
    }
    catch (InterruptedException&) {
      //LoggerFactory::getLogger().trace("Pool", "Queue is closed, pop() has thrown, breaking");
      break;
    }
    //LoggerFactory::getLogger().trace("Pool", "Task received, running");
    task.run();
    //LoggerFactory::getLogger().trace("Pool", "Making break");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    //LoggerFactory::getLogger().trace("Pool", "Checking if should  quit:" ,quit.load() ,"<<<");
  }
  //LoggerFactory::getLogger().trace("Pool", "quit flag set, decrementing workersCount");
  --workersCount;
  //LoggerFactory::getLogger().trace("Pool", "Finishing appender thread execution");
}

ThreadPoolExecutor::~ThreadPoolExecutor()
{
  shutdown();
  //awaitTermination(std::chrono::seconds(10));

  for (auto& thr : workersList) {
    thr.join();
  }
}

ThreadPoolExecutor::ThreadPoolExecutor(int32_t _corePoolSize, int32_t maxPoolSize,
  ::mariadb::Timer::Clock::duration keepAliveTime, blocking_deque<Runnable>& workQueue, ThreadFactory* _threadFactory)
  : tasksQueue(workQueue),
  corePoolSize(_corePoolSize),
  maximumPoolSize(maxPoolSize),
  allowTimeout(false),
  workersCount(0),
  worker(std::bind(&ThreadPoolExecutor::workerFunction, this)),
  threadFactory(_threadFactory),
  quit(false)
{
}

void ThreadPoolExecutor::allowCoreThreadTimeOut(bool value)
{
  // Let's assume so far, that this may be changed only before work start
  allowTimeout= value;
}

bool ThreadPoolExecutor::prestartCoreThread()
{
  for (int32_t i= workersCount.load(); i < corePoolSize; ++i) {
    workersList.emplace_back(threadFactory->newThread(worker));
    //workersList.back().detach();
    ++workersCount;
  }

  return true;
}

void ThreadPoolExecutor::shutdown()
{
  if (!quit.load()) {
    //LoggerFactory::getLogger().trace("Pool", "Beginning to shut down connection adder thread");
    quit.store(true);
    tasksQueue.close();
  }
}

template <class T, class P>
bool ThreadPoolExecutor::awaitTermination(std::chrono::duration<T, P> waitTime)
{
  return sql::awaitTermination<T, P>(waitTime, this->workersCount, workersList);
}

void ThreadPoolExecutor::execute(std::function<void()> func)
{
  tasksQueue.emplace_back(func);
}

void ThreadPoolExecutor::execute(Runnable code)
{
  tasksQueue.push_back(code);
}

//-------------------- Runnable ---------------------
Runnable& Runnable::operator=(const Runnable& other)
{
  this->codeToRun= other.codeToRun;
  return *this;
}

Runnable::Runnable(Runnable&& moved) : codeToRun(moved.codeToRun)
{
}

//-------------------- ScheduledFuture ---------------------
ScheduledFuture::ScheduledFuture(std::shared_ptr<std::atomic_bool>& flagRef) : workersQuitFlag(flagRef)
{
}

void ScheduledFuture::cancel(bool cancelType)
{
  std::shared_ptr<std::atomic_bool> existing(workersQuitFlag.lock());
  if (existing) {
    existing->store(cancelType);
  }
}

//-------------------- ScheduledThreadPoolExecutor ---------------------

ScheduledFuture* ScheduledThreadPoolExecutor::scheduleAtFixedRate(std::function<void(void)> methodToRun,
  ::mariadb::Timer::Clock::duration scheduleDelay, ::mariadb::Timer::Clock::duration delay2)
{
  ScheduledTask task(methodToRun,
    static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::seconds>(scheduleDelay).count()));
  // TODO: can do better and insert it based on execution time
  tasksQueue.push_back(task);

  if (workersCount == 0) {
    prestartCoreThread();
  }
  return new ScheduledFuture(task.canceled);
}


ScheduledThreadPoolExecutor::~ScheduledThreadPoolExecutor()
{
  for (auto& task : tasksQueue) {
    if (task.canceled) {
      task.canceled->store(true);
    }
  }
  shutdown();

  //this->awaitTermination(std::chrono::seconds(10));

  for (auto& thr : workersList) {
    thr.join();
  }
}


ScheduledThreadPoolExecutor::ScheduledThreadPoolExecutor(int32_t _corePoolSize, int32_t maxPoolSize, ThreadFactory* _threadFactory) :
  corePoolSize(_corePoolSize),
  maximumPoolSize(maxPoolSize),
  workersCount(0),
  worker(std::bind(&ScheduledThreadPoolExecutor::workerFunction, this)),
  threadFactory(_threadFactory),
  quit(false)
{
}

void ScheduledThreadPoolExecutor::workerFunction()
{
  const auto defaultTimeout= std::chrono::seconds(1);
  ScheduledTask task;

  //LoggerFactory::getLogger().trace("Pool", "Starting idles remover thread");

  while (!quit.load())
  {
    //LoggerFactory::getLogger().trace("Pool", "Idles: quit not set, requesting task");
    try {
      tasksQueue.pop(task);
    }
    catch (InterruptedException&) {
      //LoggerFactory::getLogger().trace("Pool", "Idles: queue is closed, pop() has thrown, breaking");
      break;
    }

    if (task) {
      //LoggerFactory::getLogger().trace("Pool", "Idles: valid task");
      if (task.canceled && task.canceled->load()) {
        //LoggerFactory::getLogger().trace("Pool", "Idles: canceled task, deleting");
      }
      else {
        if (task.schedulePeriod.count() == 0) {
          //LoggerFactory::getLogger().trace("Pool", "Idles: one time task, executing and deleting");
          // One time task w/out schedule. Polling the next one right away
          task.task.run();
          break;
        }
        auto now = std::chrono::steady_clock::now();

        if (now >= task.nextRunTime) {
          //LoggerFactory::getLogger().trace("Pool", "Idles: scheduled due task, executing and rescheduling");
          task.task.run();
          task.nextRunTime = now + task.schedulePeriod;
          tasksQueue.push_back(std::move(task));
        }
        else {
          //LoggerFactory::getLogger().trace("Pool", "Idles: scheduled task, too early, pushing bacj to the queue");
          // Push is like push_front
          tasksQueue.push(std::move(task));
        }

        if (task.schedulePeriod < defaultTimeout) {
          //LoggerFactory::getLogger().trace("Pool", "Idles: sleeping remaining time");
          std::this_thread::sleep_for(task.schedulePeriod);
          break;
        }
      }
    }
    //LoggerFactory::getLogger().trace("Pool", "Idles: sleeping default time");
    std::this_thread::sleep_for(defaultTimeout);
    //LoggerFactory::getLogger().trace("Pool", "Idles: Checking if should  quit:", quit.load(), "<<<");
  }
  --workersCount;
  //LoggerFactory::getLogger().trace("Pool", "Finishing idles remover thread execution, workersCount decremented");
}

bool ScheduledThreadPoolExecutor::prestartCoreThread()
{
  for (int32_t i = workersCount.load(); i < corePoolSize; ++i) {
    workersList.emplace_back(threadFactory->newThread(worker));
    //workersList.back().detach();
    ++workersCount;
  }

  return true;
}

void ScheduledThreadPoolExecutor::execute(std::function<void()> func)
{
  //tasksQueue.push(new ScheduledTask(func)); /*push= push_front*/
  //if (workersCount == 0) {
  //  prestartCoreThread();
  //}
  Runnable r(func);
  execute(r);
}

void ScheduledThreadPoolExecutor::execute(Runnable code)
{
  ScheduledTask task(code);
  tasksQueue.push(task);
  if (workersCount == 0) {
    prestartCoreThread();
  }
}

void ScheduledThreadPoolExecutor::shutdown()
{
  if (!quit.load()) {
    //LoggerFactory::getLogger().trace("Pool", "Beginning to shut down idles remover thread");
    quit.store(true);
    tasksQueue.close();
  }
}

template <class T, class P>
bool ScheduledThreadPoolExecutor::awaitTermination(std::chrono::duration<T, P> waitTime)
{
  return sql::awaitTermination<T, P>(waitTime, this->workersCount, workersList);
}

ScheduledTask::operator bool() const
{
  return canceled && !canceled->load();
}

}