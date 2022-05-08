/************************************************************************************
   Copyright (C) 2020,2021 MariaDB Corporation AB

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

#include "BinRowProtocolCapi.h"

#include "ColumnDefinition.h"
#include "ExceptionFactory.h"

namespace sql
{
namespace mariadb
{
namespace capi
{
  /**
    * Constructor.
    *
    * @param columnInformation column information.
    * @param columnInformationLength number of columns
    * @param maxFieldSize max field size
    * @param options connection options
    */
   BinRowProtocolCapi::BinRowProtocolCapi(
    std::vector<Shared::ColumnDefinition>& _columnInformation,
    int32_t _columnInformationLength,
    uint32_t _maxFieldSize,
    Shared::Options options,
    MYSQL_STMT* capiStmtHandle)
     : RowProtocol(_maxFieldSize, options)
     , stmt(capiStmtHandle)
     , columnInformation(_columnInformation)
     , columnInformationLength(_columnInformationLength)
  {
     bind.reserve(mysql_stmt_field_count(stmt));

     for (auto& columnInfo : columnInformation)
     {
       length= columnInfo->getLength();
       maxFieldSize= columnInfo->getMaxLength();
       //TODO maybe change property type in the ColumnInfo?
       bind.emplace_back();

       bind.back().buffer_type=   static_cast<enum_field_types>(columnInfo->getColumnType().getType());
       if (bind.back().buffer_type == MYSQL_TYPE_VARCHAR) {
         bind.back().buffer_type= MYSQL_TYPE_STRING;
       }
       bind.back().buffer_length= static_cast<unsigned long>(columnInfo->getColumnType().binarySize() != 0 ?
                                                         columnInfo->getColumnType().binarySize() :
                                                         getLengthMaxFieldSize());
       bind.back().buffer=        new uint8_t[bind.back().buffer_length];
       bind.back().length=        &bind.back().length_value;
       bind.back().is_null=       &bind.back().is_null_value;
       bind.back().error=         &bind.back().error_value;
     }
     maxFieldSize= 0;
     if (mysql_stmt_bind_result(stmt, bind.data())) {
       throwStmtError(stmt);
     }
  }


   BinRowProtocolCapi::~BinRowProtocolCapi()
   {
     for (auto& columnBind : bind)
     {
       delete[] (uint8_t*)columnBind.buffer;
     }
   }

  /**
    * Set length and pos indicator to requested index.
    *
    * @param newIndex index (0 is first).
    * @see <a href="https://mariadb.com/kb/en/mariadb/resultset-row/">Resultset row protocol
    *     documentation</a>
    */
  void BinRowProtocolCapi::setPosition(int32_t newIndex)
  {
    index= newIndex;
    pos= 0;

    if (buf != nullptr) {
      fieldBuf.wrap((*buf)[index], (*buf)[index].size());
      this->lastValueNull = fieldBuf ? BIT_LAST_FIELD_NOT_NULL : BIT_LAST_FIELD_NULL;
      length = static_cast<uint32_t>(fieldBuf.size());
    }
    else {
      length = bind[index].length_value;
      fieldBuf.wrap(static_cast<char*>(bind[index].buffer), length);
      this->lastValueNull = bind[index].is_null_value ? BIT_LAST_FIELD_NULL : BIT_LAST_FIELD_NOT_NULL;
    }
  }


  int32_t BinRowProtocolCapi::fetchNext()
  {
    return mysql_stmt_fetch(stmt);
  }


  void BinRowProtocolCapi::installCursorAtPosition(int32_t rowPtr)
  {
    mysql_stmt_data_seek(stmt, static_cast<unsigned long long>(rowPtr));
    //fetchNext();
  }


  SQLString* BinRowProtocolCapi::convertToString(const char* asChar, ColumnDefinition* columnInfo)
  {
    if ((lastValueNull & BIT_LAST_FIELD_NULL)!=0) {
      return nullptr;
    }

    switch (columnInfo->getColumnType().getType()) {
    case MYSQL_TYPE_STRING:
      if (getLengthMaxFieldSize() > 0) {
        return new SQLString(asChar, getLengthMaxFieldSize());
        /*buf, pos, std::min(getMaxFieldSize()*3, length), UTF_8)
        .substr(0, std::min(getMaxFieldSize(), length));*/
      }
      return new SQLString(asChar);//  UTF_8);

    case MYSQL_TYPE_BIT:
      return new SQLString(std::to_string(parseBit()));
    case MYSQL_TYPE_TINY:
      return new SQLString(zeroFillingIfNeeded(std::to_string(getInternalTinyInt(columnInfo)), columnInfo));
    case MYSQL_TYPE_SHORT:
      return new SQLString(zeroFillingIfNeeded(std::to_string(getInternalSmallInt(columnInfo)), columnInfo));
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
      return new SQLString(zeroFillingIfNeeded(std::to_string(getInternalMediumInt(columnInfo)), columnInfo));
    case MYSQL_TYPE_LONGLONG:
      if (!columnInfo->isSigned()) {
        return new SQLString(zeroFillingIfNeeded(std::to_string(getInternalULong(columnInfo)), columnInfo));
      }
      return new SQLString(zeroFillingIfNeeded(std::to_string(getInternalLong(columnInfo)), columnInfo));
    case MYSQL_TYPE_DOUBLE:
      return new SQLString(zeroFillingIfNeeded(std::to_string(getInternalDouble(columnInfo)), columnInfo));
    case MYSQL_TYPE_FLOAT:
      return new SQLString(zeroFillingIfNeeded(std::to_string(getInternalFloat(columnInfo)), columnInfo));
    case MYSQL_TYPE_TIME:
      return new SQLString(getInternalTimeString(columnInfo));
    case MYSQL_TYPE_DATE:
    {
      Date date= getInternalDate(columnInfo);// , cal, timeZone);
      if (date.empty() || date.compare(nullDate) == 0) {
        return nullptr;
      }
      return new SQLString(date);
    }
    case MYSQL_TYPE_YEAR:
    {
      if (options->yearIsDateType) {
        Date dateInter = getInternalDate(columnInfo);//, cal, timeZone);
        return (dateInter.empty() || dateInter.compare(nullDate)) == 0 ? nullptr : new SQLString(dateInter);
      }
      int32_t year= getInternalSmallInt(columnInfo);
      SQLString *result;

      if (year < 10) {
        result= new SQLString("0");
        result->append(std::to_string(year));
      }
      else {
        result= new SQLString(std::to_string(year));
      }
      return result;
    }
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATETIME:
    {
      std::unique_ptr<Timestamp> timestamp= getInternalTimestamp(columnInfo);
      if (!timestamp) {
        return nullptr;
      }
      return timestamp.release();
    }
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DECIMAL:
    case MYSQL_TYPE_GEOMETRY:
      return new SQLString(asChar, getLengthMaxFieldSize());
    case MYSQL_TYPE_NULL:
      return nullptr;
    default:
      if (getLengthMaxFieldSize() > 0) {
        return new SQLString(asChar, getLengthMaxFieldSize());
      }
      return new SQLString(asChar);
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
  std::unique_ptr<SQLString> BinRowProtocolCapi::getInternalString(ColumnDefinition* columnInfo, Calendar* cal, TimeZone* timeZone)
  {
    std::unique_ptr<SQLString> result(convertToString(fieldBuf.arr, columnInfo));

    return std::move(result);
  }

  /**
    * Get int from raw binary format.
    *
    * @param columnInfo column information
    * @return int value
    * @throws SQLException if column is not numeric or is not in Integer bounds.
    */
  int32_t BinRowProtocolCapi::getInternalInt(ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }

    int64_t value;

    switch (columnInfo->getColumnType().getType()) {
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
        return *reinterpret_cast<int32_t*>(fieldBuf.arr);
      }
      else {
        value= *reinterpret_cast<uint32_t*>(fieldBuf.arr);
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
      try {
        std::string str(fieldBuf.arr, length);
        value = std::stoll(str);
      }
      // Common parent for std::invalid_argument and std::out_of_range
      catch (std::logic_error&) {

        throw SQLException(
          "Out of range value for column '" + columnInfo->getName() + "' : value " + sql::SQLString(static_cast<char*>(fieldBuf.arr), length),
          "22003",
          1264);
      }
      break;
    default:
      throw SQLException(
        "getInt not available for data field type "
        +columnInfo->getColumnType().getCppTypeName());
    }
    rangeCheck("int32_t", INT32_MIN, INT32_MAX, value, columnInfo);
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
  int64_t BinRowProtocolCapi::getInternalLong(ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }

    int64_t value;

    try {
      switch (columnInfo->getColumnType().getType()) {

      case MYSQL_TYPE_BIT:
        return parseBit();
      case MYSQL_TYPE_TINY:
        value = getInternalTinyInt(columnInfo);
        break;
      case MYSQL_TYPE_SHORT:
      case MYSQL_TYPE_YEAR:
      {
        value = getInternalSmallInt(columnInfo);

        break;
      }
      case MYSQL_TYPE_LONG:
      case MYSQL_TYPE_INT24:
      {
        value = getInternalMediumInt(columnInfo);

        break;
      }
      case MYSQL_TYPE_LONGLONG:
      {
        value = *reinterpret_cast<uint64_t*>(fieldBuf.arr);

        if (columnInfo->isSigned()) {
          return value;
        }
        uint64_t unsignedValue = *reinterpret_cast<uint64_t*>(fieldBuf.arr);

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
        float floatValue = getInternalFloat(columnInfo);
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
        long double doubleValue = getInternalDouble(columnInfo);
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
        std::unique_ptr<BigDecimal> bigDecimal = getInternalBigDecimal(columnInfo);
        //rangeCheck("BigDecimal", static_cast<int64_t>(buf));

        return std::stoll(StringImp::get(*bigDecimal));
      }
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_VARCHAR:
      case MYSQL_TYPE_STRING:
      {
        std::string str(fieldBuf.arr, length);
        return std::stoll(str);
      }
      default:
        throw SQLException(
          "getLong not available for data field type "
          + columnInfo->getColumnType().getCppTypeName());
      }
    }
    // stoll may throw. Catching common parent for std::invalid_argument and std::out_of_range
    catch (std::logic_error&) {
      throw SQLException(
        "Out of range value for column '" + columnInfo->getName() + "' : value " + value,
        "22003",
        1264);
    }
    return value;
  }


  uint64_t BinRowProtocolCapi::getInternalULong(ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }

    int64_t value;

    switch (columnInfo->getColumnType().getType()) {

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
      value = *reinterpret_cast<int64_t*>(fieldBuf.arr);

      break;
    }
    case MYSQL_TYPE_FLOAT:
    {
      float floatValue = getInternalFloat(columnInfo);
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
      long double doubleValue = getInternalDouble(columnInfo);
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
      std::unique_ptr<BigDecimal> bigDecimal = getInternalBigDecimal(columnInfo);
      //rangeCheck("BigDecimal", static_cast<int64_t>(buf));

      return sql::mariadb::stoull(*bigDecimal);
    }
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_STRING:
    {
      std::string str(fieldBuf.arr, length);
      try {
        return sql::mariadb::stoull(str);
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
        + columnInfo->getColumnType().getCppTypeName());
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
  float BinRowProtocolCapi::getInternalFloat(ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }

    int64_t value;
    switch (columnInfo->getColumnType().getType()) {
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
        return static_cast<float>(*reinterpret_cast<int64_t*>(fieldBuf.arr));
      }
      uint64_t unsignedValue= *reinterpret_cast<uint64_t*>(fieldBuf.arr);

      return static_cast<float>(unsignedValue);
    }
    case MYSQL_TYPE_FLOAT:
      return *reinterpret_cast<float*>(fieldBuf.arr);
    case MYSQL_TYPE_DOUBLE:
      return static_cast<float>(getInternalDouble(columnInfo));
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_DECIMAL:
      try {
        char* end;
        return std::strtof(static_cast<char*>(fieldBuf.arr), &end);
        // if (errno == ERANGE) ?
      }
      // Common parent for std::invalid_argument and std::out_of_range
      catch (std::logic_error& nfe) {
        throw SQLException(
            "Incorrect format for getFloat for data field with type "
            +columnInfo->getColumnType().getCppTypeName(),
            "22003",
            1264,
            &nfe);
      }
    default:
      throw SQLException(
        "getFloat not available for data field type "
        +columnInfo->getColumnType().getCppTypeName());
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
  long double BinRowProtocolCapi::getInternalDouble(ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }
    switch (columnInfo->getColumnType().getType()) {
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
        return static_cast<long double>(*reinterpret_cast<int64_t*>(fieldBuf.arr));
      }
      return static_cast<long double>(*reinterpret_cast<uint64_t*>(fieldBuf.arr));
    }
    case MYSQL_TYPE_FLOAT:
      return getInternalFloat(columnInfo);
    case MYSQL_TYPE_DOUBLE:
      return *reinterpret_cast<double*>(fieldBuf.arr);
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_VARCHAR:
    case MYSQL_TYPE_STRING:
    case MYSQL_TYPE_DECIMAL:
      try {
        return std::stold(static_cast<char*>(fieldBuf.arr));
      }
      // Common parent for std::invalid_argument and std::out_of_range
      catch (std::logic_error& nfe) {
        throw SQLException(
            "Incorrect format for getDouble for data field with type "
            +columnInfo->getColumnType().getCppTypeName(),
            "22003",
            1264, &nfe);
      }
    default:
      throw SQLException(
        "getDouble not available for data field type "
        +columnInfo->getColumnType().getCppTypeName());
    }
  }

  /**
    * Get BigDecimal from raw binary format.
    *
    * @param columnInfo column information
    * @return BigDecimal value
    * @throws SQLException if column is not numeric
    */
  //TODO: probably unique_ptr makes sense only ofr string
  std::unique_ptr<BigDecimal> BinRowProtocolCapi::getInternalBigDecimal(ColumnDefinition* columnInfo)
  {
    std::unique_ptr<BigDecimal> nullBd;
    if (lastValueWasNull()) {
      return nullBd;
    }
    switch (columnInfo->getColumnType().getType()) {
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

        return std::unique_ptr<SQLString>(new SQLString(asChar, ptr - asChar));
      }
    }
    default:
      throw SQLException(
        "getBigDecimal not available for data field type "
        +columnInfo->getColumnType().getCppTypeName());
    }
  }


  bool isNullTimeStruct(MYSQL_TIME* mt, enum_field_types type)
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


  SQLString makeStringFromTimeStruct(MYSQL_TIME* mt, enum_field_types type, size_t decimals)
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


  Date BinRowProtocolCapi::getInternalDate(ColumnDefinition* columnInfo, Calendar* cal, TimeZone* timeZone)
  {
    if (lastValueWasNull()) {
      return nullDate;
    }
    switch (columnInfo->getColumnType().getType()) {
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_DATE:
    {
      MYSQL_TIME* mt= reinterpret_cast<MYSQL_TIME*>(fieldBuf.arr);

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
      int32_t year = *reinterpret_cast<int16_t*>(fieldBuf.arr);
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
        + columnInfo->getColumnType().getCppTypeName());
    }
  }

  void padZeroMicros(SQLString& time, uint32_t decimals)
  {
    if (decimals > 0)
    {
      time.reserve(time.length() + 1 + decimals);
      time.append('.');
      while (decimals-- > 0) {
        time.append('0');
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
  std::unique_ptr<Time> BinRowProtocolCapi::getInternalTime(ColumnDefinition* columnInfo, Calendar* cal, TimeZone* timeZone)
  {
    std::unique_ptr<Time> nullTime(new Date("00:00:00"));

    padZeroMicros(*nullTime, columnInfo->getDecimals());

    if (lastValueWasNull()) {
      return nullTime;
    }
    switch (columnInfo->getColumnType().getType()) {
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATETIME:
    {
      MYSQL_TIME* mt= reinterpret_cast<MYSQL_TIME*>(fieldBuf.arr);
      return std::unique_ptr<Time>(new Time(makeStringFromTimeStruct(mt, MYSQL_TYPE_TIME, columnInfo->getDecimals())));
    }
    case MYSQL_TYPE_DATE:
      throw SQLException("Cannot read Time using a Types::DATE field");
    case MYSQL_TYPE_STRING:
    {
      SQLString rawValue(fieldBuf.arr, length);

      /*if (rawValue.compare(*nullTime) == 0 || rawValue.compare("00:00:00") == 0) {
        lastValueNull |= BIT_LAST_ZERO_DATE;
        return nullTime;
      }*/

      return std::unique_ptr<Time>(new Time(rawValue));
    }
    default:
      throw SQLException(
        "getTime not available for data field type "
        + columnInfo->getColumnType().getCppTypeName());
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
  std::unique_ptr<Timestamp> BinRowProtocolCapi::getInternalTimestamp(ColumnDefinition* columnInfo, Calendar* userCalendar, TimeZone* timeZone)
  {
    std::unique_ptr<Timestamp> nullTs(new Timestamp("0000-00-00 00:00:00"));

    padZeroMicros(*nullTs, columnInfo->getDecimals());

    if (lastValueWasNull()) {
      return nullTs;
    }
    if (length == 0) {
      lastValueNull |=BIT_LAST_FIELD_NULL;
      return nullTs;
    }

    switch (columnInfo->getColumnType().getType()) {
    case MYSQL_TYPE_TIME:
    case MYSQL_TYPE_TIMESTAMP:
    case MYSQL_TYPE_DATETIME:
    case MYSQL_TYPE_DATE:
    {
      MYSQL_TIME* mt= reinterpret_cast<MYSQL_TIME*>(fieldBuf.arr);

      if (isNullTimeStruct(mt, MYSQL_TYPE_TIMESTAMP)) {
        lastValueNull |= BIT_LAST_ZERO_DATE;
        return nullTs;
      }
      if (columnInfo->getColumnType().getType() == MYSQL_TYPE_TIME)
      {
        mt->year= 1970;
        mt->month= 1;
        mt->day= mt->day > 0 ? mt->day : 1;
        //TODO if timeis negaive - shouldn't we deduct it?
      }

      return std::unique_ptr<Timestamp>(new Timestamp(makeStringFromTimeStruct(mt, MYSQL_TYPE_TIMESTAMP, columnInfo->getDecimals())));
    }
    case MYSQL_TYPE_VAR_STRING:
    case MYSQL_TYPE_STRING:
    {
      SQLString rawValue(fieldBuf.arr, length);

      if (rawValue.compare(*nullTs) == 0 || rawValue.compare("00:00:00") == 0) {
        lastValueNull |= BIT_LAST_ZERO_DATE;
        return nullTs;
      }

      return std::unique_ptr<Timestamp>(new Timestamp(rawValue));
    }
    default:
      throw SQLException(
        "getTimestamp not available for data field type "
        + columnInfo->getColumnType().getCppTypeName());
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
  bool BinRowProtocolCapi::getInternalBoolean(ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return false;
    }
    switch (columnInfo->getColumnType().getType()) {
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
  int8_t BinRowProtocolCapi::getInternalByte(ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }
    int64_t value;
    switch (columnInfo->getColumnType().getType()) {
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
      std::string str(fieldBuf.arr, length);
      value= std::stoll(str);
      break;
    }
    default:
      throw SQLException(
        "getByte not available for data field type "
        + columnInfo->getColumnType().getCppTypeName());
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
  int16_t BinRowProtocolCapi::getInternalShort(ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return 0;
    }

    int64_t value;
    switch (columnInfo->getColumnType().getType()) {
    case MYSQL_TYPE_BIT:
      value= parseBit();
      break;
    case MYSQL_TYPE_TINY:
      value= getInternalTinyInt(columnInfo);
      break;
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_YEAR:
      return *reinterpret_cast<int16_t*>(fieldBuf.arr);
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
      std::string str(fieldBuf.arr, length);
      value= std::stoll(str);
      break;
    }
    default:
      throw SQLException(
        "getShort not available for data field type "
        +columnInfo->getColumnType().getCppTypeName());
    }
    rangeCheck("int16_t", INT16_MIN, INT16_MAX, value, columnInfo);

    return static_cast<int16_t>(value);
  }

  /**
    * Get Time in string format from raw binary format.
    *
    * @param columnInfo column information
    * @return time value
    */
  SQLString BinRowProtocolCapi::getInternalTimeString(ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return "";
    }
    MYSQL_TIME* ts= reinterpret_cast<MYSQL_TIME*>(fieldBuf.arr);
    return makeStringFromTimeStruct(ts, MYSQL_TYPE_TIME, columnInfo->getDecimals());
  }

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  /**
  * Get Object from raw binary format.
  *
  * @param columnInfo column information
  * @param timeZone time zone
  * @return Object value
  * @throws SQLException if column type is not compatible
  */
  sql::Object* BinRowProtocolCapi::getInternalObject(ColumnDefinition* columnInfo, TimeZone* timeZone)
  {
    if (lastValueWasNull()) {
      return nullptr;
    }

    switch (columnInfo->getColumnType().getType()) {
      case MYSQL_TYPE_BIT:
        if (columnInfo->getLength()==1) {
          return buf[pos] !=0;
        }
      int8_t[] dataBit= new int8_t[length];
      memcpy(dataBit + 0, buf + pos, length);
      return dataBit;
      case MYSQL_TYPE_TINY:
        if (options->tinyInt1isBit &&columnInfo->getLength()==1) {
          return buf[pos] !=0;
        }
        return getInternalInt(columnInfo);
      case MYSQL_TYPE_LONG:
        if (!columnInfo->isSigned()) {
          return getInternalLong(columnInfo);
        }
        return getInternalInt(columnInfo);
      case MYSQL_TYPE_LONGLONG:
        if (!columnInfo->isSigned()) {
          return getInternalBigInteger(columnInfo);
        }
        return getInternalLong(columnInfo);
      case MYSQL_TYPE_DOUBLE:
        return getInternalDouble(columnInfo);
      case MYSQL_TYPE_VARCHAR:
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_STRING:
        if (columnInfo->isBinary()) {
          int8_t[] data= new int8_t[getLengthMaxFieldSize()];
          memcpy(dataBit + 0, buf + pos, length));
    return data;
        }
        return getInternalString(columnInfo, nullptr, timeZone);
      case MYSQL_TYPE_TIMESTAMP:
      case MYSQL_TYPE_DATETIME:
        return getInternalTimestamp(columnInfo, nullptr, timeZone);
      case MYSQL_TYPE_DATE:
        return getInternalDate(columnInfo, nullptr, timeZone);
      case MYSQL_TYPE_NEWDECIMAL:
        return getInternalBigDecimal(columnInfo);
      case MYSQL_TYPE_BLOB:
      case MYSQL_TYPE_LONG_BLOB:
      case MYSQL_TYPE_MEDIUM_BLOB:
      case MYSQL_TYPE_TINY_BLOB:
        int8_t[] dataBlob= new int8_t[getLengthMaxFieldSize()];
        memcpy(dataBit + 0, buf + pos, length));
      return dataBlob;
      case NULL:
        return nullptr;
      case MYSQL_TYPE_YEAR:
        if (options->yearIsDateType) {
          return getInternalDate(columnInfo, nullptr, timeZone);
        }
        return getInternalShort(columnInfo);
      case MYSQL_TYPE_SHORT:
      case MYSQL_TYPE_INT24:
        return getInternalInt(columnInfo);
      case MYSQL_TYPE_FLOAT:
        return getInternalFloat(columnInfo);
      case MYSQL_TYPE_TIME:
        return getInternalTime(columnInfo, nullptr, timeZone);
      case MYSQL_TYPE_DECIMAL:
      case JSON:
        return getInternalString(columnInfo, nullptr, timeZone);
      case MYSQL_TYPE_GEOMETRY:
        int8_t[] data= new int8_t[length];
        memcpy(dataBit + 0, buf + pos, length);
        return data;
      case MYSQL_TYPE_ENUM:
        break;
      case MYSQL_TYPE_NEWDATE:
        break;
      case MYSQL_TYPE_SET:
        break;
      default:
        break;
    }
    throw ExceptionFactory::INSTANCE.notSupported(
      "Type '"+columnInfo->getColumnType().getTypeName()+"' is not supported");
  }

  /**
    * Get BigInteger from raw binary format.
    *
    * @param columnInfo column information
    * @return BigInteger value
    * @throws SQLException if column type doesn't permit conversion or value is not in BigInteger
    *     range
    */
  BigInteger BinRowProtocolCapi::getInternalBigInteger(ColumnDefinition* columnInfo)
  {
    if (lastValueWasNull()) {
      return nullptr;
    }
    switch (columnInfo->getColumnType().getType()) {
    case MYSQL_TYPE_BIT:
      return BigInteger.valueOf(buf[pos]);
    case MYSQL_TYPE_TINY:
      return BigInteger.valueOf(columnInfo->isSigned() ? buf[pos] : (buf[pos] &0xff));
    case MYSQL_TYPE_SHORT:
    case MYSQL_TYPE_YEAR:
      int16_t valueShort= static_cast<int16_t>(((buf)[pos] &0xff)|((buf[pos +1] &0xff)<<8));
      return BigInteger.valueOf(columnInfo->isSigned() ? valueShort : (valueShort &0xffff));
    case MYSQL_TYPE_LONG:
    case MYSQL_TYPE_INT24:
      int32_t valueInt =
        ((buf[pos] &0xff)
          +((buf[pos +1] &0xff)<<8)
          +((buf[pos +2] &0xff)<<16)
          +((buf[pos +3] &0xff)<<24));
      return BigInteger.valueOf(
        ((columnInfo->isSigned())
          ? valueInt
          : (valueInt >=0) ? valueInt : valueInt &0xffffffffL));
    case MYSQL_TYPE_LONGLONG:
      int64_t value =
        ((buf[pos] &0xff)
          +(static_cast<int16_t>(((buf)[pos +1] &0xff)<<8)
            +(static_cast<int16_t>(((buf)[pos +2] &0xff)<<16)
              +(static_cast<int16_t>(((buf)[pos +3] &0xff)<<24)
                +(static_cast<int16_t>(((buf)[pos +4] &0xff)<<32)
                  +(static_cast<int16_t>(((buf)[pos +5] &0xff)<<40)
                    +(static_cast<int16_t>(((buf)[pos +6] &0xff)<<48)
                      +(static_cast<int16_t>(((buf)[pos +7] &0xff)<<56));
      if (columnInfo->isSigned()) {
        return BigInteger.valueOf(value);
      }
      else {
        return new BigInteger(
          1,
          new int8_t[]{
          (int8_t)(value >>56),
          (int8_t)(value >>48),
          (int8_t)(value >>40),
          (int8_t)(value >>32),
          (int8_t)(value >>24),
          (int8_t)(value >>16),
          (int8_t)(value >>8),
          (int8_t)value
          });
      }
    case MYSQL_TYPE_FLOAT:
      return BigInteger.valueOf(static_cast<int16_t>(((buf)
    case MYSQL_TYPE_DOUBLE:
      return BigInteger.valueOf(static_cast<int16_t>(((buf)
    case MYSQL_TYPE_NEWDECIMAL:
    case MYSQL_TYPE_DECIMAL:
      return BigInteger.valueOf(getInternalBigDecimal(columnInfo).longValue());
    default:
      return new BigInteger(new SQLString(buf, pos, length));
    }
  }

  /**
    * Get ZonedDateTime from raw binary format.
    *
    * @param columnInfo column information
    * @param clazz asked class
    * @param timeZone time zone
    * @return ZonedDateTime value
    * @throws SQLException if column type doesn't permit conversion
    */
  ZonedDateTime BinRowProtocolCapi::getInternalZonedDateTime(ColumnDefinition* columnInfo, Class* clazz, TimeZone* timeZone)
  {
    if (lastValueWasNull()) {
      return nullptr;
    }
    if (length == 0) {
      lastValueNull |=BIT_LAST_FIELD_NULL;
      return nullptr;
    }

    switch (columnInfo->getColumnType().getSqlType()) {
    case Types::MYSQL_TYPE_TIMESTAMP:
      int32_t year= ((buf[pos] &0xff)|(buf[pos +1] &0xff)<<8);
      int32_t month= buf[pos +2];
      int32_t day= buf[pos +3];
      int32_t hour= 0;
      int32_t minutes= 0;
      int32_t seconds= 0;
      int32_t microseconds= 0;

      if (length >4) {
        hour= buf[pos +4];
        minutes= buf[pos +5];
        seconds= buf[pos +6];

        if (length >7) {
          microseconds =
            ((buf[pos +7] &0xff)
              +((buf[pos +8] &0xff)<<8)
              +((buf[pos +9] &0xff)<<16)
              +((buf[pos +10] &0xff)<<24));
        }
      }

      return ZonedDateTime.of(
        year, month, day, hour, minutes, seconds, microseconds *1000, timeZone.toZoneId());

    case Types::MYSQL_TYPE_VARCHAR:
    case Types::LONGVARCHAR:
    case Types::CHAR:


      SQLString raw;
      raw.reserve(buf, pos, length);
      if (raw.startsWith("0000-00-00 00:00:00")) {
        return nullptr;
      }
      try {
        return ZonedDateTime::parse(raw, TEXT_ZONED_DATE_TIME);
      }
      catch (DateTimeParseException& dateParserEx) {
        throw SQLException(
          raw
          +" cannot be parse as ZonedDateTime. time must have \"yyyy-MM-dd[T/ ]HH:mm:ss[.S]\" "
          "with offset and timezone format (example : '2011-12-03 10:15:30+01:00[Europe/Paris]')");
      }

    default:
      throw SQLException(
        "Cannot read "
        +clazz.getName()
        +" using a "
        +columnInfo->getColumnType().getCppTypeName()
        +" field");
    }
  }

  /**
    * Get OffsetTime from raw binary format.
    *
    * @param columnInfo column information
    * @param timeZone time zone
    * @return OffsetTime value
    * @throws SQLException if column type doesn't permit conversion
    */
  OffsetTime BinRowProtocolCapi::getInternalOffsetTime(ColumnDefinition* columnInfo, TimeZone* timeZone)
  {
    if (lastValueWasNull()) {
      return nullptr;
    }
    if (length == 0) {
      lastValueNull |=BIT_LAST_FIELD_NULL;
      return nullptr;
    }

    ZoneId zoneId= timeZone.toZoneId().normalized();
    if (INSTANCEOF(zoneId, ZoneOffset)) {
      ZoneOffset zoneOffset= (ZoneOffset)zoneId;

      int32_t day= 0;
      int32_t hour= 0;
      int32_t minutes= 0;
      int32_t seconds= 0;
      int32_t microseconds= 0;

      switch (columnInfo->getColumnType().getSqlType()) {
      case Types::MYSQL_TYPE_TIMESTAMP:
        int32_t year= ((buf[pos] &0xff)|(buf[pos +1] &0xff)<<8);
        int32_t month= buf[pos +2];
        day= buf[pos +3];

        if (length >4) {
          hour= buf[pos +4];
          minutes= buf[pos +5];
          seconds= buf[pos +6];

          if (length >7) {
            microseconds =
              ((buf[pos +7] &0xff)
                +((buf[pos +8] &0xff)<<8)
                +((buf[pos +9] &0xff)<<16)
                +((buf[pos +10] &0xff)<<24));
          }
        }

        return ZonedDateTime.of(
          year, month, day, hour, minutes, seconds, microseconds *1000, zoneOffset)
          .toOffsetDateTime()
          .toOffsetTime();

      case Types::MYSQL_TYPE_TIME:
        bool negate= (buf[pos] &0xff)==0x01;

        if (length >4) {
          day =
            ((buf[pos +1] &0xff)
              +((buf[pos +2] &0xff)<<8)
              +((buf[pos +3] &0xff)<<16)
              +((buf[pos +4] &0xff)<<24));
        }

        if (length >7) {
          hour= buf[pos +5];
          minutes= buf[pos +6];
          seconds= buf[pos +7];
        }

        if (length >8) {
          microseconds =
            ((buf[pos +8] &0xff)
              +((buf[pos +9] &0xff)<<8)
              +((buf[pos +10] &0xff)<<16)
              +((buf[pos +11] &0xff)<<24));
        }

        return OffsetTime.of(
          (negate ? -1 : 1)*(day *24 +hour),
          minutes,
          seconds,
          microseconds *1000,
          zoneOffset);

      case Types::MYSQL_TYPE_VARCHAR:
      case Types::LONGVARCHAR:
      case Types::CHAR:
        SQLString raw;
        raw.reserve(buf, pos, length);
        try {
          return OffsetTime::parse(raw, DateTimeFormatter.ISO_OFFSET_TIME);
        }
        catch (DateTimeParseException& dateParserEx) {
          throw SQLException(
            raw
            +" cannot be parse as OffsetTime (format is \"HH:mm:ss[.S]\" with offset for data type \""
            +columnInfo->getColumnType()
            +"\")");
        }

      default:
        throw SQLException(
          "Cannot read "
          +typeid(OffsetTime).getName()
          +" using a "
          +columnInfo->getColumnType().getCppTypeName()
          +" field");
      }
    }

    if (options->useLegacyDatetimeCode) {

      throw SQLException(
        "Cannot return an OffsetTime for a TIME field when default timezone is '"
        +zoneId
        +"' (only possible for time-zone offset from Greenwich/UTC, such as +02:00)");
    }


    throw SQLException(
      "Cannot return an OffsetTime for a TIME field when server timezone '"
      +zoneId
      +"' (only possible for time-zone offset from Greenwich/UTC, such as +02:00)");
  }

  /**
    * Get LocalTime from raw binary format.
    *
    * @param columnInfo column information
    * @param timeZone time zone
    * @return LocalTime value
    * @throws SQLException if column type doesn't permit conversion
    */
  LocalTime BinRowProtocolCapi::getInternalLocalTime(ColumnDefinition* columnInfo, TimeZone* timeZone)
  {
    if (lastValueWasNull()) {
      return nullptr;
    }
    if (length == 0) {
      lastValueNull |=BIT_LAST_FIELD_NULL;
      return nullptr;
    }

    switch (columnInfo->getColumnType().getSqlType()) {
    case Types::MYSQL_TYPE_TIME:
      int32_t day= 0;
      int32_t hour= 0;
      int32_t minutes= 0;
      int32_t seconds= 0;
      int32_t microseconds= 0;

      bool negate= (buf[pos] &0xff)==0x01;

      if (length >4) {
        day =
          ((buf[pos +1] &0xff)
            +((buf[pos +2] &0xff)<<8)
            +((buf[pos +3] &0xff)<<16)
            +((buf[pos +4] &0xff)<<24));
      }

      if (length >7) {
        hour= buf[pos +5];
        minutes= buf[pos +6];
        seconds= buf[pos +7];
      }

      if (length >8) {
        microseconds =
          ((buf[pos +8] &0xff)
            +((buf[pos +9] &0xff)<<8)
            +((buf[pos +10] &0xff)<<16)
            +((buf[pos +11] &0xff)<<24));
      }

      return LocalTime.of(
        (negate ? -1 : 1)*(day *24 +hour), minutes, seconds, microseconds *1000);

    case Types::MYSQL_TYPE_VARCHAR:
    case Types::LONGVARCHAR:
    case Types::CHAR:

      SQLString raw;
      raw.reserve(buf, pos, length);
      try {
        return LocalTime::parse(
          raw, DateTimeFormatter.ISO_LOCAL_TIME.withZone(timeZone.toZoneId()));
      }
      catch (DateTimeParseException& dateParserEx) {
        throw SQLException(
          raw
          +" cannot be parse as LocalTime (format is \"HH:mm:ss[.S]\" for data type \""
          +columnInfo->getColumnType()
          +"\")");
      }

    case Types::MYSQL_TYPE_TIMESTAMP:
      ZonedDateTime zonedDateTime =
        getInternalZonedDateTime(columnInfo, typeid(LocalTime), timeZone);
      return zonedDateTime/*.empty() == true*/
        ? nullptr
        : zonedDateTime.withZoneSameInstant(ZoneId.systemDefault()).toLocalTime();

    default:
      throw SQLException(
        "Cannot read LocalTime using a "
        +columnInfo->getColumnType().getCppTypeName()
        +" field");
    }
  }

  /**
    * Get LocalDate from raw binary format.
    *
    * @param columnInfo column information
    * @param timeZone time zone
    * @return LocalDate value
    * @throws SQLException if column type doesn't permit conversion
    */
  LocalDate BinRowProtocolCapi::getInternalLocalDate(ColumnDefinition* columnInfo, TimeZone* timeZone)
  {
    if (lastValueWasNull()) {
      return nullptr;
    }
    if (length == 0) {
      lastValueNull |=BIT_LAST_FIELD_NULL;
      return nullptr;
    }

    switch (columnInfo->getColumnType().getSqlType()) {
    case Types::MYSQL_TYPE_DATE:
      int32_t year= ((buf[pos] &0xff)|(buf[pos +1] &0xff)<<8);
      int32_t month= buf[pos +2];
      int32_t day= buf[pos +3];
      return LocalDate.of(year, month, day);

    case Types::MYSQL_TYPE_TIMESTAMP:
      ZonedDateTime zonedDateTime =
        getInternalZonedDateTime(columnInfo, typeid(LocalDate), timeZone);
      return zonedDateTime/*.empty() == true*/
        ? nullptr
        : zonedDateTime.withZoneSameInstant(ZoneId.systemDefault()).toLocalDate();

    case Types::MYSQL_TYPE_VARCHAR:
    case Types::LONGVARCHAR:
    case Types::CHAR:

      SQLString raw;
      raw.reserve(buf, pos, length);
      if (raw.startsWith("0000-00-00")) {
        return nullptr;
      }
      try {
        return LocalDate::parse(
          raw, DateTimeFormatter.ISO_LOCAL_DATE.withZone(timeZone.toZoneId()));
      }
      catch (DateTimeParseException& dateParserEx) {
        throw SQLException(
          raw +" cannot be parse as LocalDate. time must have \"yyyy-MM-dd\" format");
      }

    default:
      throw SQLException(
        "Cannot read LocalDate using a "
        +columnInfo->getColumnType().getCppTypeName()
        +" field");
    }
  }
#endif

  /**
    * Indicate if data is binary encoded.
    *
    * @return always true.
    */
  bool BinRowProtocolCapi::isBinaryEncoded()
  {
    return true;
  }
}
}
}
