/************************************************************************************
   Copyright (C) 2020 MariaDB Corporation AB

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


#include "LoggerFactory.h"
#include "NoLogger.h"

namespace sql
{
namespace mariadb
{
  Shared::Logger LoggerFactory::NO_LOGGER= (NO_LOGGER ? NO_LOGGER : Shared::Logger(new NoLogger()));
  bool LoggerFactory::hasToLog= false;

  void LoggerFactory::init(bool mustLog)
  {
    if ((hasToLog != mustLog ) && mustLog)
    {
      if (hasToLog != mustLog)
      {
        //try
        {
          //Class.forName("org.slf4j.LoggerFactory");
          hasToLog= true;
        }
        //catch (ClassNotFoundException classNotFound)
        {
          //System.out.println("Logging cannot be activated, missing slf4j dependency");
          //hasToLog= false;
        }
      }
    }
  }

  bool LoggerFactory::initLoggersIfNeeded()
  {
    if (!NO_LOGGER) {
      NO_LOGGER.reset(new NoLogger());
    }
    return true;
  }

  Shared::Logger LoggerFactory::getLogger(const std::type_info &typeId)
  {
    static bool inited= initLoggersIfNeeded();

    if (hasToLog)
    {
      return NO_LOGGER;//Slf4JLogger(typeId);
    }
    else
    {
      return NO_LOGGER;
    }
  }
}
}
