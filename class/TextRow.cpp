/************************************************************************************
   Copyright (C) 2022,2024 MariaDB Corporation AB

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

#include "TextRow.h"

#include "ColumnDefinition.h"
#include "Exception.h"


namespace mariadb
{
/**
 * Constructor.
 *
 * @param maxFieldSize max field size
 * @param options connection options
 */
  TextRow::TextRow(MYSQL_RES* capiTextResults)
    : Row()
    , capiResults(capiTextResults, &mysql_free_result)
    , rowData(nullptr)
    , lengthArr(nullptr)
 {
 }

 /**
  * Set length and pos indicator to asked index.
  *
  * @param newIndex index (0 is first).
  */
 void TextRow::setPosition(int32_t newIndex)
 {
   index= newIndex;
   pos= 0;

   if (buf != nullptr)
   {
     fieldBuf.wrap((*buf)[index].arr, (*buf)[index].size());
     this->lastValueNull= fieldBuf ? BIT_LAST_FIELD_NOT_NULL : BIT_LAST_FIELD_NULL;
     length= static_cast<uint32_t>(fieldBuf.size());
   }
   else if (rowData) {
     this->lastValueNull= (rowData[index] == nullptr ? BIT_LAST_FIELD_NULL : BIT_LAST_FIELD_NOT_NULL);
     length= lengthArr[newIndex];
     fieldBuf.wrap(rowData[index], length);
   }
   else {
     // TODO: we need some good assert above instead of this
     throw std::runtime_error("Internal error in the TextRow class - data buffers are NULLs");
   }
 }

 /**
 * Get String from raw text format.
 *
 * @param columnInfo column information
 * @param cal calendar
 * @return String value
 * @throws SQLException if column type doesn't permit conversion
 */
 SQLString TextRow::getInternalString(const ColumnDefinition*  columnInfo)
 {
   if (lastValueWasNull()) {
     return emptyStr;
   }

   switch (columnInfo->getColumnType()) {
   case MYSQL_TYPE_BIT:
     return SQLString(std::to_string(parseBit()));
   case MYSQL_TYPE_DOUBLE:
   case MYSQL_TYPE_FLOAT:
     return zeroFillingIfNeeded(fieldBuf.arr, columnInfo);
   case MYSQL_TYPE_TIME:
     return SQLString(getInternalTimeString(columnInfo));
   case MYSQL_TYPE_DATE:
   {
     Date date(getInternalDate(columnInfo));
     if (date.empty() || date.compare(nullDate) == 0) {
       if ((lastValueNull & BIT_LAST_ZERO_DATE) != 0) {
         lastValueNull^= BIT_LAST_ZERO_DATE;
         return SQLString(fieldBuf.arr, length);
       }
       return emptyStr;
     }
     return date;
   }
   case MYSQL_TYPE_TIMESTAMP:
   case MYSQL_TYPE_DATETIME:
   {
     Timestamp timestamp= getInternalTimestamp(columnInfo);
     if (timestamp.length() == 0) {
       if ((lastValueNull & BIT_LAST_ZERO_DATE) != 0) {
         lastValueNull^= BIT_LAST_ZERO_DATE;
         return SQLString(fieldBuf.arr, length);
       }
       return emptyStr;
     }
     return timestamp;
   }
   case MYSQL_TYPE_NEWDECIMAL:
   case MYSQL_TYPE_DECIMAL:
   {
     return zeroFillingIfNeeded(getInternalBigDecimal(columnInfo), columnInfo);
   }
   case MYSQL_TYPE_NULL:
     return emptyStr;
   default:
     break;
   }

   return SQLString(fieldBuf.arr, getLengthMaxFieldSize());
 }


 Date TextRow::getInternalDate(const ColumnDefinition*  columnInfo)
 {
   if (lastValueWasNull()) {
     return nullDate;
   }

   switch (columnInfo->getColumnType()) {
   case MYSQL_TYPE_DATE:
   {
     std::vector<int32_t> datePart{ 0, 0, 0 };
     int32_t partIdx= 0;
     for (uint32_t begin= pos; begin < pos + length; begin++) {
       int8_t b= fieldBuf[begin];
       if (b == '-') {
         partIdx++;
         continue;
       }
       if (b <'0'|| b >'9') {
         throw 1;/* SQLException(
           "cannot parse data in date string '"
           + SQLString(fieldBuf, length)
           + "'")*/;
       }
       datePart[partIdx]= datePart[partIdx] *10 + b - 48;
     }

     if (datePart[0] == 0 && datePart[1] ==0 && datePart[2] == 0) {
       lastValueNull|= BIT_LAST_ZERO_DATE;
       return nullDate;
     }

     Date d(fieldBuf.arr, length);
     return d;
   }
   case MYSQL_TYPE_TIMESTAMP:
   case MYSQL_TYPE_DATETIME:
   {
     Timestamp timestamp= getInternalTimestamp(columnInfo);
     return timestamp.substr(0, 10 + (timestamp.at(0) == '-' ? 1 : 0));
   }
   case MYSQL_TYPE_TIME:
     throw 1;/*SQLException("Cannot read DATE using a TIME field")*/;

   case MYSQL_TYPE_YEAR:
   {
     int32_t year= std::stoi(fieldBuf.arr);
     if (length == 2 && columnInfo->getLength() == 2) {
       if (year < 70) {
         year +=2000;
       }
       else {
         year +=1900;
       }
     }
     std::ostringstream result;
     result << year << "-01-01";
     return result.str();
   }
   default:
   {
     std::string str(fieldBuf.arr + pos, length);
     if (isDate(str))
     {
       return str.substr(0, 10 + (str.at(0) == '-' ? 1 : 0));
     }
     else {
       throw 1/*SQLException("Could not get object as Date", "S1009")*/;
     }
   }
   }
 }
 /**
 * Get time from raw text format.
 *
 * @param columnInfo column information
 * @param cal calendar
 * @param timeZone time zone
 * @return time value
 * @throws SQLException if column type doesn't permit conversion
 */
 Time TextRow::getInternalTime(const ColumnDefinition*  columnInfo, MYSQL_TIME* dest)
 {
   static Time nullTime("00:00:00");
   if (lastValueWasNull()) {
     return nullTime;
   }

   if (columnInfo->getColumnType() == MYSQL_TYPE_TIMESTAMP
     ||columnInfo->getColumnType() == MYSQL_TYPE_DATETIME) {

     return getInternalTimestamp(columnInfo).substr(11);
   }
   else if (columnInfo->getColumnType() == MYSQL_TYPE_DATE) {

     throw 1;

   }
   else {
     SQLString raw(fieldBuf.arr + pos, length);
     std::vector<SQLString> matcher;

     if (!parseTime(raw, matcher)) {
       throw SQLException("Time format \"" + raw + "\" incorrect, must be [-]HH+:[0-59]:[0-59]");
     }

     SQLString &parts= matcher.back();
     int32_t microseconds= 0;

     if (parts.length() > 1)
     {
       std::size_t digitsCnt= parts.length();
       microseconds= std::stoi(parts.substr(1, std::min(digitsCnt, (std::size_t)6U)));

       while (digitsCnt++ < 7) {
         microseconds*= 10;
       }
     }
     if (dest != nullptr) {
       dest->hour= std::stoi(matcher[2]);
       dest->minute= std::stoi(matcher[3]);
       dest->second= std::stoi(matcher[4]);
       dest->neg= matcher[1].empty() ? '\0' : '\1';
       dest->second_part= microseconds;
     }
     return matcher[0];
   }
 }

 /**
 * Get timestamp from raw text format.
 *
 * @param columnInfo column information
 * @param userCalendar calendar
 * @param timeZone time zone
 * @return timestamp value
 * @throws SQLException if column type doesn't permit conversion
 */
 Timestamp TextRow::getInternalTimestamp(const ColumnDefinition*  columnInfo)
 {
   static Timestamp nullTs("0000-00-00 00:00:00");
   if (lastValueWasNull()) {
     return nullTs;
   }

   switch (columnInfo->getColumnType()) {
   case MYSQL_TYPE_TIMESTAMP:
   case MYSQL_TYPE_DATETIME:
   case MYSQL_TYPE_DATE:
   case MYSQL_TYPE_VARCHAR:
   case MYSQL_TYPE_VAR_STRING:
   case MYSQL_TYPE_STRING:
   {
     const std::size_t nanosIdx= 6;
     int32_t nanoBegin= -1;
     std::string nanosStr("");
     std::vector<int32_t> timestampsPart{ 0,0,0,0,0,0,0 };
     int32_t partIdx= 0;

     for (uint32_t begin= pos; begin < pos + length; begin++) {
       int8_t b= fieldBuf[begin];
       if (b == '-'|| b == ' ' || b == ':') {
         partIdx++;
         continue;
       }
       if (b == '.') {
         partIdx++;
         nanoBegin= begin;
         nanosStr.reserve(length - (nanoBegin - pos) - 1/*dot itself*/);
         continue;
       }
       if (b < '0' || b > '9') {
         throw SQLException(
           "cannot parse data in timestamp string '"
           + SQLString(fieldBuf.arr + pos, length)
           +"'");
       }
       timestampsPart[partIdx]= timestampsPart[partIdx]*10 + b - 48;
       if (partIdx == nanosIdx) {
         nanosStr.append(1, b);
       }
     }

     if (timestampsPart[0] == 0
       && timestampsPart[1] == 0
       && timestampsPart[2] == 0
       && timestampsPart[3] == 0
       && timestampsPart[4] == 0
       && timestampsPart[5] == 0
       && timestampsPart[6] == 0)
     {
       lastValueNull|= BIT_LAST_ZERO_DATE;
       return nullTs;
     }

     // fix non leading tray for nanoseconds
     if (nanoBegin > 0) {
       for (uint32_t begin= 0; begin < 9 - (pos + length - nanoBegin - 1); begin++) {
         timestampsPart[6]= timestampsPart[6]*10;
       }
     }

     std::ostringstream timestamp;
     std::locale C("C");
     timestamp.imbue(C);

     timestamp << timestampsPart[0] << "-";
     timestamp << (timestampsPart[1] < 10 ? "0" : "") << timestampsPart[1] << "-";
     timestamp << (timestampsPart[2] < 10 ? "0" : "") << timestampsPart[2] << " ";
     timestamp << (timestampsPart[3] < 10 ? "0" : "") << timestampsPart[3] << ":";
     timestamp << (timestampsPart[4] < 10 ? "0" : "") << timestampsPart[4] << ":";
     timestamp << (timestampsPart[5] < 10 ? "0" : "") << timestampsPart[5];
     
     if (timestampsPart[6] > 0) {
       /*<< (nanosStr.length() < 9 ? std::string(9 - nanosStr.length(), '0') : "")*/
       timestamp << "." << nanosStr;
     }

     return Timestamp(timestamp.str());
   }
   case MYSQL_TYPE_TIME:
   {
     Timestamp tt("1970-01-01 ");

     return tt.append(getInternalTime(columnInfo));
   }
   default:
   {
     //SQLString rawValue(fieldBuf.arr + pos, length);
     throw 1; /*SQLException(
       "Value type \""
       +columnInfo->getColumnType()
       +"\" with value \""
       + rawValue
       +"\" cannot be parse as Timestamp");*/
   }
   }
 }

 /**
  * Get int from raw text format.
  *
  * @param columnInfo column information
  * @return int value
  * @throws SQLException if column type doesn't permit conversion or not in Integer range
  */
 int32_t TextRow::getInternalInt(const ColumnDefinition*  columnInfo)
 {
   if (lastValueWasNull()) {
     return 0;
   }
   int64_t value= getInternalLong(columnInfo);
   // TODO: dirty hack(INT32_MAX->UINT32_MAX to make ODBC happy atm). Maybe it's not that dirty after all...
   rangeCheck("int32_t", INT32_MIN, UINT32_MAX, value, columnInfo);

   return static_cast<int32_t>(value);
 }

 
 /**
  * Get long from raw text format.
  *
  * @param columnInfo column information
  * @return long value
  * @throws SQLException if column type doesn't permit conversion or not in Long range (unsigned)
  */
 int64_t TextRow::getInternalLong(const ColumnDefinition*  columnInfo)
 {
   if (lastValueWasNull()) {
     return 0;
   }

   try {
     switch (columnInfo->getColumnType()) {
     case MYSQL_TYPE_FLOAT:
     case MYSQL_TYPE_DOUBLE:
     {
       long double doubleValue= safer_strtod(fieldBuf.arr + pos, length);
       if (doubleValue > static_cast<long double>(INT64_MAX)) {
         throw SQLException(
           "Out of range value for column '"
          + columnInfo->getName()
          + "' : value "
          + SQLString(fieldBuf.arr, length)
          + " is not in int64_t range",
           "22003",
           1264);
       }
       return static_cast<int64_t>(doubleValue);
     }
     case MYSQL_TYPE_BIT:
       // Workaround of the server bug MDEV-31392 - server returns string interpretation of bit field in subquery.
       // The difference in the metadata in this case is binary flag
       if (!(columnInfo->getFlags() & BINARY_FLAG)) {
         return parseBit();
       }
       // Falling down otherwise
     case MYSQL_TYPE_TINY:
     case MYSQL_TYPE_SHORT:
     case MYSQL_TYPE_YEAR:
     case MYSQL_TYPE_LONG:
     case MYSQL_TYPE_INT24:
     case MYSQL_TYPE_LONGLONG:
       return safer_strtoll(fieldBuf.arr + pos, length);
         //std::stoll(std::string(fieldBuf.arr + pos, length));
     case MYSQL_TYPE_TIMESTAMP:
     case MYSQL_TYPE_DATETIME:
     case MYSQL_TYPE_TIME:
     case MYSQL_TYPE_DATE:
       throw SQLException(
         "Conversion to integer not available for data field type "
         + std::to_string(columnInfo->getColumnType()));
     default:
       return safer_strtoll(fieldBuf.arr + pos, length);
         //std::stoll(std::string(fieldBuf.arr + pos, length));
     }

   }
   // Common parent for std::invalid_argument and std::out_of_range
   catch (std::logic_error&) {

     throw SQLException(
       "Out of range value for column '"+columnInfo->getName()+"' : value " + SQLString(fieldBuf.arr, length),
       "22003",
       1264);
   }
 }


 uint64_t TextRow::getInternalULong(const ColumnDefinition * columnInfo)
 {
   if (lastValueWasNull()) {
     return 0;
   }

   uint64_t value= 0;

   try {
     switch (columnInfo->getColumnType()) {
     case MYSQL_TYPE_FLOAT:
     case MYSQL_TYPE_DOUBLE:
     {
       long double doubleValue= safer_strtod(fieldBuf.arr, length);
       if (doubleValue < 0 || doubleValue > static_cast<long double>(UINT64_MAX)) {
         throw SQLException(
           "Out of range value for column '"
           + columnInfo->getName()
           + "' : value "
           + SQLString(fieldBuf.arr, length)
           + " is not in uint64_t range",
           "22003",
           1264);
       }
       return static_cast<uint64_t>(doubleValue);
     }
     case MYSQL_TYPE_BIT:
       return static_cast<uint64_t>(parseBit());
     case MYSQL_TYPE_TINY:
     case MYSQL_TYPE_SHORT:
     case MYSQL_TYPE_YEAR:
     case MYSQL_TYPE_LONG:
     case MYSQL_TYPE_INT24:
     case MYSQL_TYPE_LONGLONG:
       value= mariadb::stoull(fieldBuf.arr);
       break;
     case MYSQL_TYPE_TIMESTAMP:
     case MYSQL_TYPE_DATETIME:
     case MYSQL_TYPE_TIME:
     case MYSQL_TYPE_DATE:
       throw 1;
         /*"Conversion to integer not available for data field type "
         + columnInfo->getColumnType().getCppTypeName());*/
     default:
       value= mariadb::stoull(fieldBuf.arr + pos, length);
     }

   }
   // Common parent for std::invalid_argument and std::out_of_range
   catch (std::logic_error&) {
     throw 1;
       /*"Out of range value for column '" + columnInfo->getName() + "' : value " + value,
       "22003",
       1264);*/
   }

   return value;
 }

 /**
  * Get float from raw text format.
  *
  * @param columnInfo column information
  * @return float value
  * @throws SQLException if column type doesn't permit conversion or not in Float range
  */
 float TextRow::getInternalFloat(const ColumnDefinition*  columnInfo)
 {
   if (lastValueWasNull()) {
     return 0;
   }

   switch (columnInfo->getColumnType()) {
   case MYSQL_TYPE_BIT:
     return static_cast<float>(parseBit());
   case MYSQL_TYPE_TINY:
   case MYSQL_TYPE_SHORT:
   case MYSQL_TYPE_YEAR:
   case MYSQL_TYPE_LONG:
   case MYSQL_TYPE_INT24:
   case MYSQL_TYPE_FLOAT:
   case MYSQL_TYPE_DOUBLE:
   case MYSQL_TYPE_NEWDECIMAL:
   case MYSQL_TYPE_VAR_STRING:
   case MYSQL_TYPE_VARCHAR:
   case MYSQL_TYPE_STRING:
   case MYSQL_TYPE_DECIMAL:
   case MYSQL_TYPE_LONGLONG:
     try {
       return std::stof(std::string(fieldBuf.arr+pos, length));
     }
     // Common parent for std::invalid_argument and std::out_of_range
     catch (std::logic_error& nfe) {
       throw SQLException(
           "Incorrect format \""
          + SQLString(fieldBuf.arr + pos, length)
          + "\" for getFloat for data field with type "
          + std::to_string(columnInfo->getColumnType()),
           "22003",
           1264, &nfe);
     }
   default:
     throw SQLException(
       "getFloat not available for data field type "
       + std::to_string(columnInfo->getColumnType()));
   }
 }

 /**
  * Get double from raw text format.
  *
  * @param columnInfo column information
  * @return double value
  * @throws SQLException if column type doesn't permit conversion or not in Double range (unsigned)
  */
 long double TextRow::getInternalDouble(const ColumnDefinition*  columnInfo)
 {
   if (lastValueWasNull()) {
     return 0;
   }
   switch (columnInfo->getColumnType()) {
   case MYSQL_TYPE_BIT:
     return static_cast<long double>(parseBit());
   case MYSQL_TYPE_TINY:
   case MYSQL_TYPE_SHORT:
   case MYSQL_TYPE_YEAR:
   case MYSQL_TYPE_LONG:
   case MYSQL_TYPE_INT24:
   case MYSQL_TYPE_FLOAT:
   case MYSQL_TYPE_DOUBLE:
   case MYSQL_TYPE_NEWDECIMAL:
   case MYSQL_TYPE_VAR_STRING:
   case MYSQL_TYPE_VARCHAR:
   case MYSQL_TYPE_STRING:
   case MYSQL_TYPE_DECIMAL:
   case MYSQL_TYPE_LONGLONG:
     try {
       return mariadb::safer_strtod(fieldBuf.arr + pos, length);
     }
     // Common parent for std::invalid_argument and std::out_of_range
     catch (std::logic_error& nfe) {
       throw SQLException(
           "Incorrect format \""
          + SQLString(fieldBuf.arr + pos, length)
          + "\" for getDouble for data field with type "
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
  * Get BigDecimal from raw text format.
  *
  * @param columnInfo column information
  * @return BigDecimal value
  */
 BigDecimal TextRow::getInternalBigDecimal(const ColumnDefinition*  columnInfo)
 {
   if (lastValueWasNull()) {
     return emptyStr;
   }

   if (columnInfo->getColumnType() == MYSQL_TYPE_BIT) {
     return std::to_string(parseBit());
   }
   return BigDecimal(fieldBuf.arr + pos, length);
 }


  /**
  * Get boolean from raw text format.
  *
  * @param columnInfo column information
  * @return boolean value
  */
 bool TextRow::getInternalBoolean(const ColumnDefinition*  columnInfo)
 {
   if (lastValueWasNull()) {
     return false;
   }

   if (columnInfo->getColumnType() == MYSQL_TYPE_BIT) {
     return parseBit()!=0;
   }
   return convertStringToBoolean(fieldBuf.arr + pos, length);
 }

 /**
  * Get byte from raw text format.
  *
  * @param columnInfo column information
  * @return byte value
  * @throws SQLException if column type doesn't permit conversion
  */
 int8_t TextRow::getInternalByte(const ColumnDefinition*  columnInfo)
 {
   if (lastValueWasNull()) {
     return 0;
   }
   int64_t value= getInternalLong(columnInfo);
   rangeCheck("Byte", INT8_MIN, INT8_MAX, value, columnInfo);
   return static_cast<int8_t>(value);
 }

 /**
  * Get short from raw text format.
  *
  * @param columnInfo column information
  * @return short value
  * @throws SQLException if column type doesn't permit conversion or value is not in Short range
  */
 int16_t TextRow::getInternalShort(const ColumnDefinition*  columnInfo)
 {
   if (lastValueWasNull()) {
     return 0;
   }
   int64_t value= getInternalLong(columnInfo);
   // TODO: dirty hack(INT32_MAX->UINT32_MAX to make ODBC happy atm). Maybe it's not that dirty after all...
   rangeCheck("int16_t", INT16_MIN, UINT16_MAX, value, columnInfo);
   return static_cast<int16_t>(value);
 }

 /**
  * Get Time in string format from raw text format.
  *
  * @param columnInfo column information
  * @return String representation of time
  */
 SQLString TextRow::getInternalTimeString(const ColumnDefinition*  columnInfo)
 {
   if (lastValueWasNull()) {
     return "";
   }

   SQLString rawValue(fieldBuf.arr + pos, length);
   if (rawValue.compare("0000-00-00") == 0) {
     return "";
   }

   return rawValue;
 }


 int32_t TextRow::fetchNext()
 {
   //Assuming it is called only for the case of the data from server, and not constructed text results
   rowData= mysql_fetch_row(this->capiResults.get());
   if (rowData)
   {
     lengthArr= mysql_fetch_lengths(this->capiResults.get());
     return 0;
   }
   lengthArr= nullptr;
   return 1;
 }


 void TextRow::installCursorAtPosition(int32_t rowPtr)
 {
   mysql_data_seek(capiResults.get(), static_cast<unsigned long long>(rowPtr));
 }

 /**
  * Indicate if data is binary encoded.
  *
  * @return always false.
  */
 bool TextRow::isBinaryEncoded()
 {
   return false;
 }


 void TextRow::cacheCurrentRow(std::vector<mariadb::bytes_view>& rowDataCache, std::size_t columnCount)
 {
   rowDataCache.clear();
   for (std::size_t i= 0; i < columnCount; ++i) {
     rowDataCache.emplace_back(lengthArr[i], rowData[i]);
   }
 }
} // namespace mariadb
