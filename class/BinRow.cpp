/************************************************************************************
   Copyright (C) 2022, 2024 MariaDB Corporation AB

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


#include <sstream>

#include "BinRow.h"

#include "ColumnDefinition.h"
#include "Exception.h"

namespace mariadb
{
  /**
    * Constructor.
    *
    * @param columnInformation column information.
    * @param columnInformationLength number of columns
    * @param maxFieldSize max field size
    * @param options connection options
    */
   BinRow::BinRow(
    const std::vector<ColumnDefinition>& _columnInformation,
    int32_t _columnInformationLength,
    MYSQL_STMT* capiStmtHandle)
     : Row()
     , stmt(capiStmtHandle)
     , columnInformation(_columnInformation)
     , columnInformationLength(_columnInformationLength)
  {
     bind.reserve(mysql_stmt_field_count(stmt));

     for (auto& columnInfo : columnInformation)
     {
       length= columnInfo.getLength();
       bind.emplace_back();
       ColumnDefinition::fieldDeafaultBind(columnInfo, bind.back());
     }
   }


   BinRow::~BinRow()
   {
     for (auto& columnBind : bind)
     {
       //delete[] (uint8_t*)columnBind.buffer;
     }
   }

  /**
    * Set length and pos indicator to requested index.
    *
    * @param newIndex index (0 is first).
    * @see <a href="https://mariadb.com/kb/en/mariadb/resultset-row/">Resultset row protocol
    *     documentation</a>
    */
  void BinRow::setPosition(int32_t newIndex)
  {
    index= newIndex;
    pos= 0;

    if (buf != nullptr) {
      fieldBuf.wrap((*buf)[index], (*buf)[index].size());
      this->lastValueNull= fieldBuf ? BIT_LAST_FIELD_NOT_NULL : BIT_LAST_FIELD_NULL;
      length= static_cast<uint32_t>(fieldBuf.size());
    }
    else {
      length= bind[index].length_value;
      fieldBuf.wrap(static_cast<char*>(bind[index].buffer), length);
      this->lastValueNull= bind[index].is_null_value ? BIT_LAST_FIELD_NULL : BIT_LAST_FIELD_NOT_NULL;
    }
  }


  int32_t BinRow::fetchNext()
  {
    return mysql_stmt_fetch(stmt);
  }


  void BinRow::installCursorAtPosition(int32_t rowPtr)
  {
    mysql_stmt_data_seek(stmt, static_cast<unsigned long long>(rowPtr));
    //fetchNext();
  }


  SQLString BinRow::convertToString(const char* asChar, const ColumnDefinition* columnInfo)
  {
    if ((lastValueNull & BIT_LAST_FIELD_NULL)!=0) {
      return emptyStr;
    }

    switch (columnInfo->getColumnType()) {
    case MYSQL_TYPE_STRING:
      if (getLengthMaxFieldSize() > 0) {
        return SQLString(asChar, getLengthMaxFieldSize());
        /*buf, pos, std::min(getMaxFieldSize()*3, length), UTF_8)
        .substr(0, std::min(getMaxFieldSize(), length));*/
      }
      return SQLString(asChar);//  UTF_8);

    case MYSQL_TYPE_BIT:
      return SQLString(std::to_string(parseBit()));
    case MYSQL_TYPE_TINY:
      return SQLString(zeroFillingIfNeeded(std::to_string(getInternalTinyInt(columnInfo)), columnInfo));
    case MYSQL_TYPE_SHORT:
      return SQLString(zeroFillingIfNeeded(std::to_string(getInternalSmallInt(columnInfo)), columnInfo));
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
      return SQLString(zeroFillingIfNeeded(std::to_string(getInternalMediumInt(columnInfo)), columnInfo));
    case MYSQL_TYPE_LONGLONG:
      if (!columnInfo->isSigned()) {
        return SQLString(zeroFillingIfNeeded(std::to_string(getInternalULong(columnInfo)), columnInfo));
      }
      return SQLString(zeroFillingIfNeeded(std::to_string(getInternalLong(columnInfo)), columnInfo));
    case MYSQL_TYPE_DOUBLE:
      return SQLString(zeroFillingIfNeeded(std::to_string(getInternalDouble(columnInfo)), columnInfo));
    case MYSQL_TYPE_FLOAT:
      return SQLString(zeroFillingIfNeeded(std::to_string(getInternalFloat(columnInfo)), columnInfo));
    case MYSQL_TYPE_TIME:
      return SQLString(getInternalTimeString(columnInfo));
    case MYSQL_TYPE_DATE:
    {
      Date date= getInternalDate(columnInfo);// , cal, timeZone);
      if (date.empty() || date.compare(nullDate) == 0) {
        return emptyStr;
      }
      return date;
    }
    case MYSQL_TYPE_YEAR:
    {
      int32_t year= getInternalSmallInt(columnInfo);

      if (year < 10) {
        SQLString result("0");
        return result.append(std::to_string(year));
      }
      else {
        return std::to_string(year);
      }
    }
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATETIME:
    {
      return getInternalTimestamp(columnInfo);
    }
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_GEOMETRY:
      return SQLString(asChar, getLengthMaxFieldSize());
    case MYSQL_TYPE_NULL:
      return nullptr;
    default:
      if (getLengthMaxFieldSize() > 0) {
        return SQLString(asChar, getLengthMaxFieldSize());
      }
      return SQLString(asChar);
    }
  }

  /**
    * Get string from raw binary format.
    *
    * @param columnInfo column information
    * @param cal calendar
    * @param timeZone time zone
    * @return String value of raw bytes
    * @throws SQLException if conversion failed
    */
  SQLString BinRow::getInternalString(const ColumnDefinition* columnInfo)
  {
    return std::move(convertToString(fieldBuf.arr, columnInfo));
  }

  /**
    * Get int from raw binary format.
    *
    * @param columnInfo column information
    * @return int value
    * @throws SQLException if column is not numeric or is not in Integer bounds.
    */
  int32_t BinRow::getInternalInt(const ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }

    int64_t value;

    switch (columnInfo->getColumnType()) {
    case MYSQL_TYPE_BIT:
      value= parseBit();
      break;
    case MYSQL_TYPE_TINY:
      value= getInternalTinyInt(columnInfo);
      break;
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_YEAR:
      value= getInternalSmallInt(columnInfo);
      break;
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
      if (columnInfo->isSigned()) {
        return *reinterpret_cast<const int32_t*>(fieldBuf.arr);
      }
      else {
        value= *reinterpret_cast<const uint32_t*>(fieldBuf.arr);
      }
      break;
    case MYSQL_TYPE_LONGLONG:
      value= getInternalLong(columnInfo);
      break;
    case MYSQL_TYPE_FLOAT:
      value= static_cast<int64_t>(getInternalFloat(columnInfo));
        break;
    case MYSQL_TYPE_DOUBLE:
      value= static_cast<int64_t>(getInternalDouble(columnInfo));
        break;
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DECIMAL:
      value= getInternalLong(columnInfo);
      break;
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_STRING:
      value= safer_strtoll(fieldBuf.arr, length);
      // Common parent for std::invalid_argument and std::out_of_range
      /*catch (std::logic_error&) {

        throw SQLException(
          "Out of range value for column '" + columnInfo->getName() + "' : value " + SQLString(fieldBuf.arr, length),
          "22003",
          1264);
      }*/
      break;
    default:
      throw SQLException(
        "getInt not available for data field type "
        + std::to_string(columnInfo->getColumnType()));
    }
    // TODO: dirty hack(INT32_MAX->UINT32_MAX to make ODBC happy atm). Maybe it's not that dirty after all...
    rangeCheck("int32_t", INT32_MIN, UINT32_MAX, value, columnInfo);
    return static_cast<int32_t>(value);
  }

  /**
    * Get long from raw binary format.
    *
    * @param columnInfo column information
    * @return long value
    * @throws SQLException if column is not numeric or is not in Long bounds (for big unsigned
    *     values)
    */
  int64_t BinRow::getInternalLong(const ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }

    int64_t value;

    try {
      switch (columnInfo->getColumnType()) {

      case MYSQL_TYPE_BIT:
        return parseBit();
      case MYSQL_TYPE_TINY:
        value= getInternalTinyInt(columnInfo);
        break;
      case MYSQL_TYPE_SHORT:
      case MYSQL_TYPE_YEAR:
      {
        value= getInternalSmallInt(columnInfo);

        break;
      }
      case MYSQL_TYPE_LONG:
      case MYSQL_TYPE_INT24:
      {
        value= getInternalMediumInt(columnInfo);

        break;
      }
      case MYSQL_TYPE_LONGLONG:
      {
        value= *reinterpret_cast<const uint64_t*>(fieldBuf.arr);

        if (columnInfo->isSigned()) {
          return value;
        }
        uint64_t unsignedValue= *reinterpret_cast<const uint64_t*>(fieldBuf.arr);

        if (unsignedValue > static_cast<uint64_t>(INT64_MAX)) {
          throw SQLException(
            "Out of range value for column '"
            + columnInfo->getName()
            + "' : value "
            + std::to_string(unsignedValue)
            + " is not in int64_t range",
            "22003",
            1264);
        }
        return unsignedValue;
      }
      case MYSQL_TYPE_FLOAT:
      {
        float floatValue= getInternalFloat(columnInfo);
        if (floatValue > static_cast<float>(INT64_MAX)) {
          throw SQLException(
            "Out of range value for column '"
            + columnInfo->getName()
            + "' : value "
            + std::to_string(floatValue)
            + " is not in int64_t range",
            "22003",
            1264);
        }
        return static_cast<int64_t>(floatValue);
      }
      case MYSQL_TYPE_DOUBLE:
      {
        long double doubleValue= getInternalDouble(columnInfo);
        if (doubleValue > static_cast<long double>(INT64_MAX)) {
          throw SQLException(
            "Out of range value for column '"
            + columnInfo->getName()
            + "' : value "
            + std::to_string(doubleValue)
            + " is not in int64_t range",
            "22003",
            1264);
        }
        return static_cast<int64_t>(doubleValue);
      }
      case MYSQL_TYPE_NEWDECIMAL:
      case MYSQL_TYPE_DECIMAL:
      {
        //rangeCheck("BigDecimal", static_cast<int64_t>(buf));
        return std::stoll(getInternalBigDecimal(columnInfo));
      }
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_VARCHAR:
      case MYSQL_TYPE_STRING:
      {
        return safer_strtoll(fieldBuf.arr, length);
      }
      default:
        throw SQLException(
          "getLong not available for data field type "
          + std::to_string(columnInfo->getColumnType()));
      }
    }
    // stoll may throw. Catching common parent for std::invalid_argument and std::out_of_range
    catch (std::logic_error&) {
      throw SQLException(
        "Out of range value for column '" + columnInfo->getName() + "' : value " + std::to_string(value),
        "22003",
        1264);
    }
    return value;
  }


  uint64_t BinRow::getInternalULong(const ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }

    int64_t value;

    switch (columnInfo->getColumnType()) {

    case MYSQL_TYPE_BIT:
      return static_cast<int64_t>(parseBit());
    case MYSQL_TYPE_TINY:
      value= getInternalTinyInt(columnInfo);
      break;
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_YEAR:
    {
      value= getInternalSmallInt(columnInfo);

      break;
    }
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
    {
      value= getInternalMediumInt(columnInfo);

      break;
    }
    case MYSQL_TYPE_LONGLONG:
    {
      value= *reinterpret_cast<const int64_t*>(fieldBuf.arr);

      break;
    }
    case MYSQL_TYPE_FLOAT:
    {
      float floatValue= getInternalFloat(columnInfo);
      if (floatValue < 0 || floatValue > static_cast<long double>(UINT64_MAX)) {
        throw SQLException(
          "Out of range value for column '"
          + columnInfo->getName()
          + "' : value "
          + std::to_string(floatValue)
          + " is not in int64_t range",
          "22003",
          1264);
      }
      return static_cast<uint64_t>(floatValue);
    }
    case MYSQL_TYPE_DOUBLE:
    {
      long double doubleValue= getInternalDouble(columnInfo);
      if (doubleValue < 0 || doubleValue > static_cast<long double>(UINT64_MAX)) {
        throw SQLException(
          "Out of range value for column '"
          + columnInfo->getName()
          + "' : value "
          + std::to_string(doubleValue)
          + " is not in int64_t range",
          "22003",
          1264);
      }
      return static_cast<int64_t>(doubleValue);
    }
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DECIMAL:
    {
      //rangeCheck("BigDecimal", static_cast<int64_t>(buf));
      return mariadb::stoull(getInternalBigDecimal(columnInfo));
    }
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_STRING:
    {
      std::string str(fieldBuf.arr, length);
      try {
        return mariadb::stoull(str);
      }
      // Common parent for std::invalid_argument and std::out_of_range
      catch (std::logic_error&) {
        throw SQLException(
          "Out of range value for column '"
          + columnInfo->getName()
          + "' : value "
          + str
          + " is not in int64_t range",
          "22003",
          1264);
      }
    }
    default:
      throw SQLException(
        "getLong not available for data field type "
        + std::to_string(columnInfo->getColumnType()));
    }

    if (columnInfo->isSigned() && value < 0) {
      throw SQLException(
        "Out of range value for column '"
        + columnInfo->getName()
        + "' : value "
        + std::to_string(value)
        + " is not in int64_t range",
        "22003",
        1264);
    }

    return static_cast<uint64_t>(value);
  }

  /**
    * Get float from raw binary format.
    *
    * @param columnInfo column information
    * @return float value
    * @throws SQLException if column is not numeric or is not in Float bounds.
    */
  float BinRow::getInternalFloat(const ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }

    int64_t value;
    switch (columnInfo->getColumnType()) {
    case MYSQL_TYPE_BIT:
      return static_cast<float>(parseBit());
    case MYSQL_TYPE_TINY:
      value= getInternalTinyInt(columnInfo);
      break;
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_YEAR:
      value= getInternalSmallInt(columnInfo);
      break;
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
      value= getInternalMediumInt(columnInfo);
      break;
    case MYSQL_TYPE_LONGLONG:
    {
      if (columnInfo->isSigned()) {
        return static_cast<float>(*reinterpret_cast<const int64_t*>(fieldBuf.arr));
      }
      uint64_t unsignedValue= *reinterpret_cast<const uint64_t*>(fieldBuf.arr);

      return static_cast<float>(unsignedValue);
    }
    case MYSQL_TYPE_FLOAT:
      return *reinterpret_cast<const float*>(fieldBuf.arr);
    case MYSQL_TYPE_DOUBLE:
      return static_cast<float>(getInternalDouble(columnInfo));
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_DECIMAL:
      try {
        char* end;
        return std::strtof(fieldBuf.arr, &end);
        // if (errno == ERANGE) ?
      }
      // Common parent for std::invalid_argument and std::out_of_range
      catch (std::logic_error& nfe) {
        throw SQLException(
            "Incorrect format for getFloat for data field with type "
            + std::to_string(columnInfo->getColumnType()),
            "22003",
            1264,
            &nfe);
      }
    default:
      throw SQLException(
        "getFloat not available for data field type "
        + std::to_string(columnInfo->getColumnType()));
    }
    return static_cast<float>(value);
  }

  /**
    * Get double from raw binary format.
    *
    * @param columnInfo column information
    * @return double value
    * @throws SQLException if column is not numeric or is not in Double bounds (unsigned columns).
    */
  long double BinRow::getInternalDouble(const ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }
    switch (columnInfo->getColumnType()) {
    case MYSQL_TYPE_BIT:
      return static_cast<long double>(parseBit());
    case MYSQL_TYPE_TINY:
      return getInternalTinyInt(columnInfo);
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_YEAR:
      return getInternalSmallInt(columnInfo);
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
      return static_cast<long double>(getInternalMediumInt(columnInfo));
    case MYSQL_TYPE_LONGLONG:
    {
      if (columnInfo->isSigned()) {
        return static_cast<long double>(*reinterpret_cast<const int64_t*>(fieldBuf.arr));
      }
      return static_cast<long double>(*reinterpret_cast<const uint64_t*>(fieldBuf.arr));
    }
    case MYSQL_TYPE_FLOAT:
      return getInternalFloat(columnInfo);
    case MYSQL_TYPE_DOUBLE:
      return *reinterpret_cast<const double*>(fieldBuf.arr);
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_DECIMAL:
      try {
        return safer_strtod(fieldBuf.arr, length);
      }
      // Common parent for std::invalid_argument and std::out_of_range
      catch (std::logic_error& nfe) {
        throw SQLException(
            "Incorrect format for getDouble for data field with type "
            + std::to_string(columnInfo->getColumnType()),
            "22003",
            1264, &nfe);
      }
    default:
      throw SQLException(
        "getDouble not available for data field type "
        + std::to_string(columnInfo->getColumnType()));
    }
  }

  /**
    * Get BigDecimal from raw binary format.
    *
    * @param columnInfo column information
    * @return BigDecimal value
    * @throws SQLException if column is not numeric
    */
  
  BigDecimal BinRow::getInternalBigDecimal(const ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return emptyStr;
    }
    switch (columnInfo->getColumnType()) {
    case MYSQL_TYPE_BIT:
    case MYSQL_TYPE_TINY:
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_YEAR:
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
    case MYSQL_TYPE_LONGLONG:
    case MYSQL_TYPE_FLOAT:
    case MYSQL_TYPE_DOUBLE:
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_NEWDECIMAL:
       return getInternalString(columnInfo);
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_STRING:
    {
      if (length > 0)
      {
        const char *asChar= fieldBuf.arr, *ptr= asChar, *end= asChar + length;
        if (*ptr == '+' || *ptr == '-') {
          ++ptr;
        }
        //TODO: make it better
        while (ptr < end && ( *ptr == '.' || isdigit(*ptr))) ++ptr;

        return SQLString(asChar, ptr - asChar);
      }
    }
    default:
      throw SQLException(
        "getBigDecimal not available for data field type "
        + std::to_string(columnInfo->getColumnType()));
    }
  }


  bool isNullTimeStruct(const MYSQL_TIME* mt, enum_field_types type)
  {
    bool isNull= (mt->year == 0 && mt->month == 0 && mt->day == 0);

    switch (type) {
    case MYSQL_TYPE_TIME:
      return false;
    case MYSQL_TYPE_DATE:
      return isNull;
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATETIME:
      return (isNull && mt->hour == 0 && mt->minute == 0 && mt->second == 0 && mt->second_part == 0);
    default:
      return false;
    }
    return false;
  }


  SQLString makeStringFromTimeStruct(const MYSQL_TIME* mt, enum_field_types type, size_t decimals)
  {
    std::ostringstream out;
    if (mt->neg != 0)
    {
      out << "-";
    }

    switch (type) {
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_DATE:
      out << mt->year << "-" << (mt->month < 10 ? "0" : "") << mt->month << "-" << (mt->day < 10 ? "0" : "") << mt->day;
      if (type == MYSQL_TYPE_DATE) {
        break;
      }
      out << " ";
    case MYSQL_TYPE_TIME:
      out << (mt->hour < 10 ? "0" : "") << mt->hour << ":" << (mt->minute < 10 ? "0" : "") << mt->minute << ":" << (mt->second < 10 ? "0" : "") << mt->second;

      if (mt->second_part != 0 && decimals > 0)
      {
        SQLString digits(std::to_string(mt->second_part));

        if (digits.length() > std::min(decimals, (size_t)6U))
        {
          digits= digits.substr(0, 6);
        }
        size_t padZeros= std::min(decimals, 6 - digits.length());

        out << ".";

        if (digits.length() + padZeros > 6)
        {
          digits= digits.substr(0, 6 - padZeros);
        }

        while (padZeros--) {
          out << "0";
        }

        out << digits.c_str();
      }
      break;
    default:
      // clang likes options for all enum members. Other types should not normally happen here. Probably would be better to throw here an exception
      return emptyStr;
    }
    return out.str();
  }


  Date BinRow::getInternalDate(const ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return nullDate;
    }
    switch (columnInfo->getColumnType()) {
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_DATE:
    {
      const MYSQL_TIME* mt= reinterpret_cast<const MYSQL_TIME*>(fieldBuf.arr);

      if (isNullTimeStruct(mt, MYSQL_TYPE_DATE)) {
        lastValueNull |= BIT_LAST_ZERO_DATE;
        return nullDate;
      }

      return makeStringFromTimeStruct(mt, MYSQL_TYPE_DATE, columnInfo->getDecimals());
    }
    case MYSQL_TYPE_TIME:
      throw SQLException("Cannot read Date using a Types::TIME field");
    case MYSQL_TYPE_STRING:
    {
      SQLString rawValue(fieldBuf.arr, length);

      if (rawValue.compare(nullDate) == 0) {
        lastValueNull |= BIT_LAST_ZERO_DATE;
        return nullDate;
      }

      return Date(rawValue);
    }
    case MYSQL_TYPE_YEAR:
    {
      int32_t year= *reinterpret_cast<const int16_t*>(fieldBuf.arr);
      if (length == 2 && columnInfo->getLength() == 2) {
        if (year < 70) {
          year += 2000;
        }
        else {
          year += 1900;
        }
      }
      std::ostringstream result;
      result << year << "-01-01";
      return result.str();
    }
    default:
      throw SQLException(
        "getDate not available for data field type "
        + std::to_string(columnInfo->getColumnType()));
    }
  }

  void padZeroMicros(SQLString& time, uint32_t decimals)
  {
    if (decimals > 0)
    {
      time.reserve(time.length() + 1 + decimals);
      time.append(1, '.');
      while (decimals-- > 0) {
        time.append(1, '0');
      }
    }
  }
  /**
    * Get time from raw binary format.
    *
    * @param columnInfo column information
    * @param cal calendar
    * @param timeZone time zone
    * @return Time value
    * @throws SQLException if column cannot be converted to Time
    */
  Time BinRow::getInternalTime(const ColumnDefinition* columnInfo, MYSQL_TIME* dest)
  {
    std::reference_wrapper<Time> nullTime= std::ref(Row::nullTime);
    Time nullTimeWithMicros;

    if (columnInfo->getDecimals() > 0) {
      nullTimeWithMicros= Row::nullTime;
      padZeroMicros(nullTimeWithMicros, columnInfo->getDecimals());
      nullTime= std::ref(nullTimeWithMicros);
    }

    if (lastValueWasNull()) {
      return nullTime;
    }
    switch (columnInfo->getColumnType()) {
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATETIME:
    {
      const MYSQL_TIME* mt= reinterpret_cast<const MYSQL_TIME*>(fieldBuf.arr);
      return makeStringFromTimeStruct(mt, MYSQL_TYPE_TIME, columnInfo->getDecimals());
    }
    case MYSQL_TYPE_DATE:
      throw SQLException("Cannot read Time using a Types::DATE field");
    case MYSQL_TYPE_STRING:
    {
      SQLString rawValue(fieldBuf.arr, length);

      if (rawValue.compare(nullTime) == 0 || rawValue.compare(Row::nullTime) == 0) {
        lastValueNull|= BIT_LAST_ZERO_DATE;
        return nullTime;
      }
      return rawValue;
    }
    default:
      throw SQLException(
        "getTime not available for data field type "
        + std::to_string(columnInfo->getColumnType()));
    }

    return nullTime;
  }

  /**
    * Get timestamp from raw binary format.
    *
    * @param columnInfo column information
    * @param userCalendar user calendar
    * @param timeZone time zone
    * @return timestamp value
    * @throws SQLException if column type is not compatible
    */
  Timestamp BinRow::getInternalTimestamp(const ColumnDefinition* columnInfo)
  {
    std::reference_wrapper<Timestamp> nullTs= std::ref(Row::nullTs);
    Timestamp nullTsWithMicros;

    if (columnInfo->getDecimals() > 0) {
      nullTsWithMicros= Row::nullTs;
      padZeroMicros(nullTsWithMicros, columnInfo->getDecimals());
      nullTs= std::ref(nullTsWithMicros);
    }

    if (lastValueWasNull()) {
      return nullTs;
    }
    if (length == 0) {
      lastValueNull|= BIT_LAST_FIELD_NULL;
      return nullTs;
    }

    switch (columnInfo->getColumnType()) {
    case MYSQL_TYPE_TIME:
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_DATE:
    {
      MYSQL_TIME* mt= const_cast<MYSQL_TIME*>(reinterpret_cast<const MYSQL_TIME*>(fieldBuf.arr));

      if (isNullTimeStruct(mt, MYSQL_TYPE_TIMESTAMP)) {
        lastValueNull |= BIT_LAST_ZERO_DATE;
        return nullTs;
      }
      if (columnInfo->getColumnType() == MYSQL_TYPE_TIME)
      {
        mt->year= 1970;
        mt->month= 1;
        mt->day= mt->day > 0 ? mt->day : 1;
        //TODO if time is negaive - shouldn't we deduct it? with move to bytes_view - is it ok to write to the struct?
      }

      return makeStringFromTimeStruct(mt, MYSQL_TYPE_TIMESTAMP, columnInfo->getDecimals());
    }
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_STRING:
    {
      SQLString rawValue(fieldBuf.arr, length);

      if (rawValue.compare(nullTs) == 0 || rawValue.compare("00:00:00") == 0) {
        lastValueNull |= BIT_LAST_ZERO_DATE;
        return nullTs;
      }

      return rawValue;
    }
    default:
      throw SQLException(
        "getTimestamp not available for data field type "
        + std::to_string(columnInfo->getColumnType()));
    }

    return nullTs;
  }

  /**
    * Get boolean from raw binary format.
    *
    * @param columnInfo column information
    * @return boolean value
    * @throws SQLException if column type doesn't permit conversion
    */
  bool BinRow::getInternalBoolean(const ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return false;
    }
    switch (columnInfo->getColumnType()) {
    case MYSQL_TYPE_BIT:
      return parseBit()!=0;
    case MYSQL_TYPE_TINY:
      return getInternalTinyInt(columnInfo)!=0;
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_YEAR:
      return getInternalSmallInt(columnInfo)!=0;
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
      return getInternalMediumInt(columnInfo)!=0;
    case MYSQL_TYPE_LONGLONG:
      if (columnInfo->isSigned()) {
        return getInternalLong(columnInfo) != 0;
      }
      else {
        return getInternalULong(columnInfo) != 0;
      }
    case MYSQL_TYPE_FLOAT:
      return getInternalFloat(columnInfo)!=0;
    case MYSQL_TYPE_DOUBLE:
      return getInternalDouble(columnInfo)!=0;
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DECIMAL:
    {
      return getInternalLong(columnInfo) != 0;
    }
    default:
      return convertStringToBoolean(fieldBuf.arr, length);
    }
  }

  /**
    * Get byte from raw binary format.
    *
    * @param columnInfo column information
    * @return byte value
    * @throws SQLException if column type doesn't permit conversion
    */
  int8_t BinRow::getInternalByte(const ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }
    int64_t value;
    switch (columnInfo->getColumnType()) {
    case MYSQL_TYPE_BIT:
      value= parseBit();
      break;
    case MYSQL_TYPE_TINY:
      value= getInternalTinyInt(columnInfo);
      break;
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_YEAR:
      value= getInternalSmallInt(columnInfo);
      break;
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
      value= getInternalMediumInt(columnInfo);
      break;
    case MYSQL_TYPE_LONGLONG:
      value= getInternalLong(columnInfo);
      break;
    case MYSQL_TYPE_FLOAT:
      value= static_cast<int64_t>(getInternalFloat(columnInfo));
        break;
    case MYSQL_TYPE_DOUBLE:
      value= static_cast<int64_t>(getInternalFloat(columnInfo));
        break;
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DECIMAL:
      value= getInternalLong(columnInfo);
      break;
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_STRING:
    {
      value= safer_strtoll(fieldBuf.arr, length);
      break;
    }
    default:
      throw SQLException(
        "getByte not available for data field type "
        + std::to_string(columnInfo->getColumnType()));
    }
    rangeCheck("byte", INT8_MIN, INT8_MAX, value, columnInfo);
    return static_cast<int8_t>(value);
  }

  /**
    * Get short from raw binary format.
    *
    * @param columnInfo column information
    * @return short value
    * @throws SQLException if column type doesn't permit conversion
    */
  int16_t BinRow::getInternalShort(const ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }

    int64_t value;
    switch (columnInfo->getColumnType()) {
    case MYSQL_TYPE_BIT:
      value= parseBit();
      break;
    case MYSQL_TYPE_TINY:
      value= getInternalTinyInt(columnInfo);
      break;
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_YEAR:
      return *reinterpret_cast<const int16_t*>(fieldBuf.arr);
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
      value= getInternalMediumInt(columnInfo);
      break;
    case MYSQL_TYPE_LONGLONG:
      value= getInternalLong(columnInfo);
      break;
    case MYSQL_TYPE_FLOAT:
      value= static_cast<int64_t>(getInternalFloat(columnInfo));
      break;
    case MYSQL_TYPE_DOUBLE:
      value= static_cast<int64_t>(getInternalDouble(columnInfo));
      break;
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DECIMAL:
      value= getInternalLong(columnInfo);
      break;
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_STRING:
    {
      value= safer_strtoll(fieldBuf.arr, length);
      break;
    }
    default:
      throw SQLException(
        "getShort not available for data field type "
        + std::to_string(columnInfo->getColumnType()));
    }
    rangeCheck("int16_t", INT16_MIN, UINT16_MAX, value, columnInfo);
    return static_cast<int16_t>(value);
  }

  /**
    * Get Time in string format from raw binary format.
    *
    * @param columnInfo column information
    * @return time value
    */
  SQLString BinRow::getInternalTimeString(const ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return "";
    }
    const MYSQL_TIME* ts= reinterpret_cast<const MYSQL_TIME*>(fieldBuf.arr);
    return makeStringFromTimeStruct(ts, MYSQL_TYPE_TIME, columnInfo->getDecimals());
  }

  /**
    * Indicate if data is binary encoded.
    *
    * @return always true.
    */
  bool BinRow::isBinaryEncoded()
  {
    return true;
  }


  void BinRow::cacheCurrentRow(std::vector<mariadb::bytes_view>& rowDataCache, std::size_t columnCount)
  {
    rowDataCache.clear();
    for (std::size_t i= 0; i < columnCount; ++i) {
      auto& b= bind[i];
      if (b.is_null_value != '\0') {
        rowDataCache.emplace_back();
      }
      else {
        // If we could guarantee, that "b.length && *b.length > 0 ? *b.length :" is for current row, we could use it
        rowDataCache.emplace_back(b.buffer_length);
        b.buffer= const_cast<char*>(rowDataCache.back().arr);
        mysql_stmt_fetch_column(stmt, &b, static_cast<unsigned int>(i), 0);
      }
    }
  }
} // namespace mariadb
