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
#include <fstream>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <thread>
#include <cstring>
#include <sstream>

#include "SimpleLogger.h"
#define MACPPLOGNAME "mariadbccpp.log"
namespace sql
{
namespace mariadb
{
  static std::mutex outputLock;
  static std::ofstream file;
  static std::ostream *log= nullptr;
  uint32_t SimpleLogger::level= 0;


  std::string& getDefaultLogFilename(std::string& name)
  {
#ifdef _WIN32
#define MAPATHSEP '\\'

    const char *defaultLogDir= "c:";
    char *tmp= getenv("USERPROFILE");
    if (tmp)
    {
      defaultLogDir= tmp;
    }
    tmp= getenv("TMP");
    if (tmp)
    {
      defaultLogDir= tmp;
    }
#else
#define MAPATHSEP '/'
    const char *defaultLogDir="/tmp";
    char *tmp= getenv("HOME");

    if (tmp)
    {
      defaultLogDir= tmp;
    }
#endif
    name.reserve(std::strlen(defaultLogDir) + sizeof(MACPPLOGNAME) + 1 /*'/'*/);
    (name= defaultLogDir).append(1, MAPATHSEP).append(MACPPLOGNAME);
    return name;
  }


  void SimpleLogger::setLogFilename(const std::string& name)
  {
    /*if (name.compare("cerr") == 0 || name.compare("std::err") == 0) {
      log= &std::cerr;
      return;
    }
    else */if (name.empty() || name.compare(MACPPLOGNAME) == 0) {
      std::string defName;
      file.open(getDefaultLogFilename(defName), std::ios_base::app);
    }
    else {
      file.open(name, std::ios_base::app);
    }
    log= &file;
  }


  void putTimestamp(std::ostream &e)
  {
    auto tp= std::chrono::system_clock::now();
    auto t= std::chrono::system_clock::to_time_t(tp);
    auto fractional = tp.time_since_epoch() - std::chrono::seconds(t);
    std::tm* tm = std::localtime(&t);
    char buf[10];
    std::strftime(buf, 80, "%H:%M:%S", tm);

    // Ooooh.... some old compilers do not have put_time. TODO: make check try_compile in cmake, and defines something to be able to use put_time primarily
    //e << std::put_time(std::localtime(&t), "%H:%M:%S") << "." << std::chrono::duration_cast<std::chrono::milliseconds>(fractional).count();
    e << buf << "." << std::chrono::duration_cast<std::chrono::milliseconds>(fractional).count();
  }


  SimpleLogger::SimpleLogger(const char* loggedClassName)
  {
    std::size_t len= std::strlen(loggedClassName), offset= 0;
    if (len > 5) {
      if (std::strncmp(loggedClassName, "class", 5) == 0 ) {
        offset= 6;
      }
      else if (std::strncmp(loggedClassName, "struct", 6) == 0) {
        offset= 7;
      }
    }
    signature.reserve(len - offset + 3/*"[]"*/);
    signature.append(1, '[').append(loggedClassName + offset, len - offset).append(1,']');
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
      putTimestamp(*log);
      *log << " " << std::this_thread::get_id() << " " << signature << " TRACE - " << msg << std::endl;
    }
  }

  bool SimpleLogger::isDebugEnabled() {
    return (level > INFO);
  }

  void SimpleLogger::debug(const SQLString& msg) {
    if (level > INFO) {
      std::unique_lock<std::mutex> lock(outputLock);
      putTimestamp(*log);
      *log << " " << std::this_thread::get_id() << " " << signature << " DEBUG - " << msg << std::endl;
    }
  }

  void SimpleLogger::debug(const SQLString& msg, std::exception& e) {
    if (level > INFO) {
      std::unique_lock<std::mutex> lock(outputLock);
      putTimestamp(*log);
      *log << " " << std::this_thread::get_id() << " " << signature << " DEBUG - " << msg << ", Exception: " << e.what() << std::endl;
    }
  }

  void SimpleLogger::debug(const SQLString& msg, const SQLString& tag, int32_t total, int64_t active, int32_t pending) {
    if (level > INFO) {
      std::unique_lock<std::mutex> lock(outputLock);
      putTimestamp(*log);
      *log << " " << std::this_thread::get_id() << " " << signature << " DEBUG - " << msg << ", " << tag << ", " << total << "/" << active << "/" << pending << std::endl;
    }
  }

  bool SimpleLogger::isInfoEnabled() {
    return (level > WARNING);
  }

  void SimpleLogger::info(const SQLString& msg) {
    if (level > WARNING) {
      std::unique_lock<std::mutex> lock(outputLock);
      putTimestamp(*log);
      *log << " " << std::this_thread::get_id() << " " << signature << " INFO - " << msg << std::endl;
    }
  }

  bool SimpleLogger::isWarnEnabled() {
    return (level > ERROR);
  }

  void SimpleLogger::warn(const SQLString& msg) {
    if (level > ERROR) {
      std::unique_lock<std::mutex> lock(outputLock);
      putTimestamp(*log);
      *log << " " << std::this_thread::get_id() << " " << signature << " WARNING - " << msg << std::endl;
    }
  }

  bool SimpleLogger::isErrorEnabled() {
    return (level > 0);
  }

  void SimpleLogger::error(const SQLString& msg) {
    if (level) {
      std::unique_lock<std::mutex> lock(outputLock);
      putTimestamp(*log);
      *log << " " << std::this_thread::get_id() << " " << signature << " ERROR - " << msg << std::endl;
    }
  }

  void SimpleLogger::error(const SQLString& msg, SQLException& e) {
    if (level) {
      std::unique_lock<std::mutex> lock(outputLock);
      putTimestamp(*log);
      *log << " " << std::this_thread::get_id() << " " << signature << " ERROR - " << msg << ", Exception: [" << e.getSQLStateCStr() << "]" <<
        e.getMessage() << "(" << e.getErrorCode() << ")" << std::endl;
    }
  }
  void SimpleLogger::error(const SQLString& msg, MariaDBExceptionThrower& t) {
    if (level) {
      std::unique_lock<std::mutex> lock(outputLock);
      auto& e= *t.getException();
      putTimestamp(*log);
      *log << " " << std::this_thread::get_id() << " " << signature << " ERROR - " << msg << ", Exception: [" << e.getSQLStateCStr() << "]" <<
        e.getMessage() << "(" << e.getErrorCode() << ")" << std::endl;
    }
  }
}
}
