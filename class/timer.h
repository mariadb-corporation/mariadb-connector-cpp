/************************************************************************************
   Copyright (C) 2024 MariaDB Corporation plc

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not see <http://www.gnu.org/licenses>
   or write to the Free Software Foundation, Inc.,
   51 Franklin St., Fifth Floor, Boston, MA 02110, USA
*************************************************************************************/

#ifndef _MARIADB_TIMER_H_
#define _MARIADB_TIMER_H_

#include <chrono>
#include <list>
#include <vector>
#include <map>
#include <numeric>
#include <stdexcept>
#include <algorithm>

namespace mariadb
{
// These probably should be moved to framework. However 2 basic classes could be used in the driver.
class StopTimer
{
public:
  using Clock= std::chrono::steady_clock;
  using Duration= std::chrono::nanoseconds;
  using TimePoint= std::chrono::time_point<Clock>;

protected:
  TimePoint start;
  Duration duration;

public:
 
  StopTimer()
    : start(Clock::now())
  {}

  Duration stop()
  {
    duration= std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start);
    return duration;
  }

  void reset()
  {
    duration= Duration(0);
    start= Clock::now();
  }

  operator uint32_t() const
  {
    return static_cast<uint32_t>(duration.count());
  }

  operator Duration() const
  {
    return duration;
  }

  Duration operator()() const {
    return duration;
  }
};


//StopTimer::Duration operator +(const StopTimer& op1, const StopTimer& op2)
//{
//  return static_cast<StopTimer::Duration>(op1) + static_cast<StopTimer::Duration>(op2);
//}

// A small extension - with ttl and method to say if it's over
class Timer : public StopTimer
{
Clock::duration ttl;

public:
  Timer(Clock::duration ttl)
    : ttl(ttl)
  {}


  bool over() const
  {
    return Clock::now() - start >= ttl;
  }

  
  Duration left() const
  {
    auto passed= Clock::now() - start;

    if (passed >= ttl)
    {
      return Duration(0);
    }
    return std::chrono::duration_cast<Duration>(ttl - passed);
   
  }
};

// --------------------------------------------------------------------
class EntityTimer : public mariadb::Timer
{
public:
  using Timers= std::list<StopTimer>;
  using Stats= std::map<uint32_t, Timers>;

private:
  Stats timers;

public:
  EntityTimer(Duration ttl)
    : Timer(ttl)
  {}

  // Create and start a new timer for given id
  void start(uint32_t id)
  {
    /*auto& it= timers.find(id);
    if (it == timers.end())
    {
      auto& p= timers.insert(std::make_pair(id, Timers()));
      p.first->second.emplace_back();
    }
    else
    {
      it->second.emplace_back();
    }*/
    timers[id].emplace_back();
  }


  void stop(uint32_t id)
  {
    timers[id].back().stop();
  }

  const Stats& getStats() const {
    return timers;
  }

  // Min on timer
  static Duration minVal(const Timers& data)
  {
    if (data.empty())
    {
      return Duration(0);
    }
    return *std::min_element(data.begin(), data.end());
  }

  // Max on timer
  static Duration maxVal(const Timers& data)
  {
    if (data.empty())
    {
      return Duration(0);
    }
    return *std::max_element(data.begin(), data.end());
  }

  // Average on timer
  static Duration average(const Timers& data)
  {
    return total(data) / data.size();
  }

  // Sum on timer
  static Duration total(const Timers& data)
  {
    Duration sum(0);
    for (auto& it : data)
    {
      sum+= it;
    }
    return sum;// std::accumulate(data.begin(), data.end(), Duration(0));
  }

  // Expected value
  static Duration expected(const Timers& data)
  {
    if (data.empty()) {
      throw std::invalid_argument("Empty data");
    }

    // This seems to be wrong :innocent:
    std::vector<double> probabilities(data.size());
    double sum= 0.0;
    for (const auto& d : data) {
      sum+= d / 1000000.0;
    }
    std::size_t i= 0;
    for (const auto& d : data) {
      probabilities[i++]= (d / 1000000.0) / sum;
    }

    // the expected value - the weighted sum of the durations
    double expected= 0.0;
    i= 0;
    for (const auto& d : data) {
      expected+= probabilities[i] * (d / 1000000.0);
    }

    return Duration(static_cast<long long>(expected * 1000000));
  }
};

} // namespace mariadb
#endif //_MARIADB_TIMER_H_
