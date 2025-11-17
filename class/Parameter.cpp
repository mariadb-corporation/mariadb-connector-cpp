/************************************************************************************
   Copyright (C) 2023 MariaDB Corporation AB

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

#include <cstring>

#include "Parameter.h"
#include "ColumnDefinition.h"

namespace mariadb
{
  const char BINARY_INTRODUCER[]= { '_','b','i','n','a','r','y',' ','\'','\0' };
  const char QUOTE= '\'';
  void escapeData(const char* in, std::size_t len, bool noBackslashEscapes, SQLString& out);

  long typeLen[MYSQL_TYPE_TIME2+1]= {0, 1, 2, 4, 4, 8, 0, sizeof(MYSQL_TIME), 8, 4
    , sizeof(MYSQL_TIME) /*MYSQL_TYPE_DATE*/
    , sizeof(MYSQL_TIME), sizeof(MYSQL_TIME), 2, sizeof(MYSQL_TIME), -static_cast<long>(sizeof(char*)), 1, sizeof(MYSQL_TIME)
    , sizeof(MYSQL_TIME)/*MYSQL_TYPE_DATETIME2*/
    , sizeof(MYSQL_TIME) };

  /* TODO: Should go elsewhere ? */
  long getTypeBinLength(enum_field_types type)
  {
    if (type <= MYSQL_TYPE_TIME2 && typeLen[type] > 0) {
      return typeLen[type];
    }
    return 0;
  }

  void* Parameter::getBuffer(MYSQL_BIND& param, std::size_t row)
  {
    /* For bulk we have array of pointers. Need to change MYSQL_TIME part to be less dirtyhacky */
    if (param.buffer_type > MYSQL_TYPE_TIME2 ||
      typeLen[param.buffer_type] < 0 ||
      typeLen[param.buffer_type] == sizeof(MYSQL_TIME)) {
      return (static_cast<void**>(param.buffer))[row];
    }
    else {
      return static_cast<char*>(param.buffer) + typeLen[param.buffer_type] * row;
    }
  }


  unsigned long Parameter::getLength(MYSQL_BIND& param, std::size_t row)
  {
    if (param.length != nullptr) {
      return param.length[row];
    }
    else {
      return getTypeBinLength(param.buffer_type);
    }
    return 0;
  }

  void addDate(SQLString& query, MYSQL_TIME* date)
  {
    query.append(std::to_string(date->year));
    query.append(1, '-');
    if (date->month < 10) {
      query.append(1, '0');
    }
    query.append(std::to_string(date->month));
    query.append(1, '-');
    if (date->day < 10) {
      query.append(1, '0');
    }
    query.append(std::to_string(date->day));
  }

  void addTime(SQLString& query, MYSQL_TIME* time)
  {
    if (time->hour < 10) {
      query.append(1, '0');
    }
    query.append(std::to_string(time->hour));
    query.append(1, ':');
    if (time->minute < 10) {
      query.append(1, '0');
    }
    query.append(std::to_string(time->minute));
    query.append(1, ':');
    if (time->second < 10) {
      query.append(1, '0');
    }
    query.append(std::to_string(time->second));
    if (time->second_part > 0) {
      query.append(1, '.');
      SQLString mks(std::to_string(time->second_part));
      for (std::size_t i= mks.length(); i < 6; ++i) query.append(1, '0');
      query.append(mks);
    }
  }


  SQLString& Parameter::toString(SQLString& query, void* value, enum enum_field_types type, unsigned long length, bool noBackslashEscapes)
  {
    if (length > 0 && (type > MYSQL_TYPE_TIME2 || typeLen[type] < 0))
    {
      switch (type) {
      case MARIADB_BINARY_TYPES:
        query.append(BINARY_INTRODUCER);
        break;
      default:
        query.append(1, QUOTE);
      }
      escapeData(static_cast<const char*>(value), static_cast<std::size_t>(length), noBackslashEscapes, query);
      query.append(1, QUOTE);
    }
    else {
      switch (type)
      {
      case MYSQL_TYPE_BIT:
      case MYSQL_TYPE_TINY:
        query.append(std::to_string(*static_cast<int8_t*>(value)));
        break;
      case MYSQL_TYPE_YEAR:
      case MYSQL_TYPE_SHORT:
        query.append(std::to_string(*static_cast<int16_t*>(value)));
        break;
      case MYSQL_TYPE_LONG:
        query.append(std::to_string(*static_cast<int32_t*>(value)));
        break;
      case MYSQL_TYPE_FLOAT:
        query.append(std::to_string(*static_cast<float*>(value)));
        break;
      case MYSQL_TYPE_DOUBLE:
        query.append(std::to_string(*static_cast<double*>(value)));
        break;
      case MYSQL_TYPE_NULL:
        query.append("NULL");
        break;
      case MYSQL_TYPE_LONGLONG:
        query.append(std::to_string(*static_cast<int64_t*>(value)));
        break;
      case MYSQL_TYPE_INT24:
        query.append(std::to_string(*static_cast<float*>(value)));
        break;
      case MYSQL_TYPE_DATE:
      case MYSQL_TYPE_NEWDATE:
      {
        MYSQL_TIME* date= static_cast<MYSQL_TIME*>(value);
        query.append(1, QUOTE);
        addDate(query, date);
        query.append(1, QUOTE);
        break;
      }
      case MYSQL_TYPE_TIME:
      case MYSQL_TYPE_TIME2:
      {
        MYSQL_TIME* time= static_cast<MYSQL_TIME*>(value);
        query.append(1, QUOTE);
        addTime(query, time);
        query.append(1, QUOTE);
        break;
      }
      case MYSQL_TYPE_TIMESTAMP:
      case MYSQL_TYPE_DATETIME:
      case MYSQL_TYPE_DATETIME2:
      case MYSQL_TYPE_TIMESTAMP2:
      {
        MYSQL_TIME* timestamp= static_cast<MYSQL_TIME*>(value);
        query.append(1, QUOTE);
        addDate(query, timestamp);
        query.append(1, ' ');
        addTime(query, timestamp);
        query.append(1, QUOTE);
        break;
      }
      default:
      {
        const char* asString= static_cast<const char*>(value);
        query.append(1, QUOTE);
        //escapeData(asString, length > 0 ? length : std::strlen(asString), noBackslashEscapes, query);
        if (length > 0) {
          escapeData(asString, length, noBackslashEscapes, query);
        }
        query.append(1, QUOTE);
      }
      }
    }
    return query;
  }


  SQLString& Parameter::toString(SQLString& query, MYSQL_BIND& param, bool noBackslashEscapes)
  {
    return toString(query, param.buffer, param.buffer_type, param.buffer_length, noBackslashEscapes);
  }


  SQLString& Parameter::toString(SQLString& query, MYSQL_BIND& param, std::size_t row, bool noBackslashEscapes)
  {
    if (param.u.indicator != nullptr) {
      switch (param.u.indicator[row]) {
      case STMT_INDICATOR_NULL:
        return query.append("NULL");
      case STMT_INDICATOR_IGNORE:
        return query.append("DEFAULT");
      }
    }
    return toString(query, getBuffer(param, row), param.buffer_type, getLength(param, row), noBackslashEscapes);
  }


  std::size_t Parameter::getApproximateStringLength(MYSQL_BIND& param, std::size_t row)
  {
    unsigned long length= getLength(param, row);
    if (length > 0 && (param.buffer_type > MYSQL_TYPE_TIME2 || typeLen[param.buffer_type] < 0)) {
      std::size_t result= length*2;
      switch (param.buffer_type) {
      case MARIADB_BINARY_TYPES:
        result+= sizeof(BINARY_INTRODUCER); // sizeof contains terminating null which we would anyway add for the closing quote
        break;
      default:
        result+= 2;
      }
      return result;
    }
    else {
      switch (param.buffer_type)
      {
      case MYSQL_TYPE_BIT:
        return 1;
      case MYSQL_TYPE_TINY:
        return 4; // 3 + sign
      case MYSQL_TYPE_YEAR:
      case MYSQL_TYPE_SHORT:
        return 6; // 5 + sign
      case MYSQL_TYPE_LONG:
        return 11;
      case MYSQL_TYPE_FLOAT:
        return 7;
      case MYSQL_TYPE_DOUBLE:
        return 15;
      case MYSQL_TYPE_NULL:
        return 4;
      case MYSQL_TYPE_LONGLONG:
        return 20;
      case MYSQL_TYPE_INT24:
        return 8;
      case MYSQL_TYPE_DATE:
      case MYSQL_TYPE_NEWDATE:
        return 12;
      case MYSQL_TYPE_TIME:
      case MYSQL_TYPE_TIME2:
        return 11;
      case MYSQL_TYPE_TIMESTAMP:
      case MYSQL_TYPE_DATETIME:
      case MYSQL_TYPE_DATETIME2:
      case MYSQL_TYPE_TIMESTAMP2:
        return 21;
      default:
        if (length > 0) {
          return length*2 + 2;
        }
        else {
          return -1;
        }
      }
    }
  }
} // namespace mariadb
