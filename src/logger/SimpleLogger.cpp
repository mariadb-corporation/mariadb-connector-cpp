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

#include <iostream>
#include <mutex>
#include <chrono>
#include <iomanip>

#include "SimpleLogger.h"

namespace sql
{
namespace mariadb
{
  static std::mutex outputLock;
  uint32_t SimpleLogger::level= 0;

  void putTimestamp(std::ostream &e)
  {
    auto tp= std::chrono::system_clock::now();
    auto t= std::chrono::system_clock::to_time_t(tp);
    auto fractional = tp.time_since_epoch() - std::chrono::seconds(t);
    e << std::put_time(std::localtime(&t), "%H:%M:%S") << "." << std::chrono::duration_cast<std::chrono::milliseconds>(fractional).count();
  }


  SimpleLogger::SimpleLogger(const char* loggedClassName)
  {
    std::size_t len= strlen(loggedClassName), off= 0;
    if (len > 5) {
      if (strncmp(loggedClassName, "class", 5) == 0 ) {
        off= 6;
      }
      else if (strncmp(loggedClassName, "struct", 6) == 0) {
        off= 7;
      }
    }
    signature.reserve(len - off + 3/*"[]"*/);
    signature.append(1, '[').append(loggedClassName + off, len - off).append(1,']');
  }


  void SimpleLogger::setLoggingLevel(uint32_t _level)
  {
    level= _level;
  }

  bool SimpleLogger::isTraceEnabled() {
    return (level > DEBUG);
  }

  void SimpleLogger::trace(const SQLString& msg) {
    if (level > DEBUG) {
      std::unique_lock<std::mutex> lock(outputLock);
      putTimestamp(std::cerr);
      std::cerr << " " << std::this_thread::get_id() << " " << signature << " TRACE - " << msg << std::endl;
    }
  }

  bool SimpleLogger::isDebugEnabled() {
    return (level > INFO);
  }

  void SimpleLogger::debug(const SQLString& msg) {
    if (level > INFO) {
      std::unique_lock<std::mutex> lock(outputLock);
      putTimestamp(std::cerr);
      std::cerr << " " << std::this_thread::get_id() << " " << signature << " DEBUG - " << msg << std::endl;
    }
  }

  void SimpleLogger::debug(const SQLString& msg, std::exception& e) {
    if (level > INFO) {
      std::unique_lock<std::mutex> lock(outputLock);
      putTimestamp(std::cerr);
      std::cerr << " " << std::this_thread::get_id() << " " << signature << " DEBUG - " << msg << ", Exception: " << e.what() << std::endl;
    }
  }

  void SimpleLogger::debug(const SQLString& msg, const SQLString& tag, int32_t total, int64_t active, int32_t pending) {
    if (level > INFO) {
      std::unique_lock<std::mutex> lock(outputLock);
      putTimestamp(std::cerr);
      std::cerr << " " << std::this_thread::get_id() << " " << signature << " DEBUG - " << msg << ", " << tag << ", " << total << "/" << active << "/" << pending << std::endl;
    }
  }

  bool SimpleLogger::isInfoEnabled() {
    return (level > WARNING);
  }

  void SimpleLogger::info(const SQLString& msg) {
    if (level > WARNING) {
      std::unique_lock<std::mutex> lock(outputLock);
      putTimestamp(std::cerr);
      std::cerr << " " << std::this_thread::get_id() << " " << signature << " INFO - " << msg << std::endl;
    }
  }

  bool SimpleLogger::isWarnEnabled() {
    return (level > ERROR);
  }

  void SimpleLogger::warn(const SQLString& msg) {
    if (level > ERROR) {
      std::unique_lock<std::mutex> lock(outputLock);
      putTimestamp(std::cerr);
      std::cerr << " " << std::this_thread::get_id() << " " << signature << " WARNING - " << msg << std::endl;
    }
  }

  bool SimpleLogger::isErrorEnabled() {
    return (level > 0);
  }

  void SimpleLogger::error(const SQLString& msg) {
    if (level) {
      std::unique_lock<std::mutex> lock(outputLock);
      putTimestamp(std::cerr);
      std::cerr << " " << std::this_thread::get_id() << " " << signature << " ERROR - " << msg << std::endl;
    }
  }

  void SimpleLogger::error(const SQLString& msg, SQLException& e) {
    if (level) {
      std::unique_lock<std::mutex> lock(outputLock);
      putTimestamp(std::cerr);
      std::cerr << " " << std::this_thread::get_id() << " " << signature << " ERROR - " << msg << ", Exception: [" << e.getSQLStateCStr() << "]" <<
        e.getMessage() << "(" << e.getErrorCode() << ")" << std::endl;
    }
  }
  void SimpleLogger::error(const SQLString& msg, MariaDBExceptionThrower& t) {
    if (level) {
      std::unique_lock<std::mutex> lock(outputLock);
      auto& e= *t.getException();
      putTimestamp(std::cerr);
      std::cerr << " " << std::this_thread::get_id() << " " << signature << " ERROR - " << msg << ", Exception: [" << e.getSQLStateCStr() << "]" <<
        e.getMessage() << "(" << e.getErrorCode() << ")" << std::endl;
    }
  }
}
}
