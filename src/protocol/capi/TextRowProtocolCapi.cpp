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


#include <sstream>

#include "TextRowProtocolCapi.h"

#include "ExceptionFactory.h"
#include "ColumnType.h"
#include "ColumnDefinition.h"

namespace sql
{
namespace mariadb
{
namespace capi
{

/**
 * Constructor.
 *
 * @param maxFieldSize max field size
 * @param options connection options
 */
  TextRowProtocolCapi::TextRowProtocolCapi(int32_t maxFieldSize, Shared::Options options, MYSQL_RES* capiTextResults)
    : RowProtocol(maxFieldSize, options)
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
 void TextRowProtocolCapi::setPosition(int32_t newIndex)
 {
   index= newIndex;

   pos= 0;

   if (buf != nullptr) {
     fieldBuf.wrap((*buf)[index], (*buf)[index].size());
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
 * @param timeZone time zone
 * @return String value
 * @throws SQLException if column type doesn't permit conversion
 */
 std::unique_ptr<SQLString> TextRowProtocolCapi::getInternalString(ColumnDefinition* columnInfo, Calendar* cal, TimeZone* timeZone)
 {
   std::unique_ptr<SQLString> nullStr;
   if (lastValueWasNull()) {
     return nullStr;
   }

   switch (columnInfo->getColumnType().getType()) {
   case MYSQL_TYPE_BIT:
     return std::unique_ptr<SQLString>(new SQLString(std::to_string(parseBit())));
   case MYSQL_TYPE_DOUBLE:
   case MYSQL_TYPE_FLOAT:
     return std::unique_ptr<SQLString>(new SQLString(zeroFillingIfNeeded(fieldBuf.arr, columnInfo)));
   case MYSQL_TYPE_TIME:
     return std::unique_ptr<SQLString>(new SQLString(getInternalTimeString(columnInfo)));
   case MYSQL_TYPE_DATE:
   {
     Date date(getInternalDate(columnInfo, cal, timeZone));
     if (date.empty() || date.compare(nullDate) == 0) {
       if ((lastValueNull & BIT_LAST_ZERO_DATE) != 0) {
         lastValueNull^= BIT_LAST_ZERO_DATE;
         return std::unique_ptr<SQLString>(new SQLString(fieldBuf, length));
       }
       return nullStr;
     }
     return std::unique_ptr<SQLString>(new SQLString(date));
   }
   case MYSQL_TYPE_YEAR:
   {
     if (options->yearIsDateType) {
       Date date1(getInternalDate(columnInfo, cal, timeZone));
       if (date1.empty() || date1.compare(nullDate) == 0) {
         return nullStr;
       }
       else {
         return std::unique_ptr<SQLString>(new SQLString(date1));
       }
     }
     break;
   }
   case MYSQL_TYPE_TIMESTAMP:
   case MYSQL_TYPE_DATETIME:
   {
     std::unique_ptr<Timestamp> timestamp= getInternalTimestamp(columnInfo, cal, timeZone);
     if (!timestamp) {
       if ((lastValueNull & BIT_LAST_ZERO_DATE)!=0) {
         lastValueNull ^=BIT_LAST_ZERO_DATE;
         return std::unique_ptr<SQLString>(new SQLString(fieldBuf, length));
       }
       return nullStr;
     }
     return timestamp;
   }
   case MYSQL_TYPE_NEWDECIMAL:
   case MYSQL_TYPE_DECIMAL:
   {
     std::unique_ptr<BigDecimal> bigDecimal= getInternalBigDecimal(columnInfo);
     if (!bigDecimal) {
       return nullStr;
     }
     else {
       return std::unique_ptr<SQLString>(new SQLString(zeroFillingIfNeeded(*bigDecimal, columnInfo)));
     }
   }
   case MYSQL_TYPE_NULL:
     return nullStr;
   default:
     break;
   }

   return std::unique_ptr<SQLString>(new SQLString(fieldBuf, getLengthMaxFieldSize()));
 }


 Date TextRowProtocolCapi::getInternalDate(ColumnDefinition* columnInfo, Calendar* cal, TimeZone* timeZone)
 {
   if (lastValueWasNull()) {
     return nullDate;
   }

   switch (columnInfo->getColumnType().getType()) {
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
         throw SQLException(
           "cannot parse data in date string '"
           + SQLString(fieldBuf, length)
           + "'");
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
     std::unique_ptr<Timestamp> timestamp= getInternalTimestamp(columnInfo, cal, timeZone);
     if (!timestamp) {
       return nullDate;
     }
     return timestamp->substr(0, 10 + (timestamp->at(0) == '-' ? 1 : 0));
   }
   case MYSQL_TYPE_TIME:
     throw SQLException("Cannot read DATE using a Types::TIME field");

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
     if (std::regex_match(str, dateRegex))
     {
       return str.substr(0, 10 + (str.at(0) == '-' ? 1 : 0));
     }
     else {
       throw SQLException("Could not get object as Date", "S1009");
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
 std::unique_ptr<Time> TextRowProtocolCapi::getInternalTime(ColumnDefinition* columnInfo, Calendar* cal, TimeZone* timeZone)
 {
   std::unique_ptr<Time> nullTime(new Time("00:00:00"));
   if (lastValueWasNull()) {
     return nullTime;
   }

   if (columnInfo->getColumnType()==ColumnType::TIMESTAMP
     ||columnInfo->getColumnType()==ColumnType::DATETIME) {

     std::unique_ptr<Timestamp> timestamp= getInternalTimestamp(columnInfo, cal, timeZone);
     if (!timestamp) {
       return nullTime;
     }
     else {
       return std::unique_ptr<Time>(new Time(timestamp->substr(11)));
     }
   }
   else if (columnInfo->getColumnType() == ColumnType::DATE) {

     throw SQLException("Cannot read Time using a Types::DATE field");

   }
   else {
     std::string raw(fieldBuf.arr + pos, length);
     std::smatch matcher;

     if (!std::regex_search(raw, matcher, timeRegex)) {
       throw SQLException("Time format \""+raw +"\" incorrect, must be HH:mm:ss");
     }
     bool negate= !matcher[1].str().empty();

     int32_t hour= std::stoi(matcher[2]);
     int32_t minutes= std::stoi(matcher[3]);
     int32_t seconds= std::stoi(matcher[4]);
     std::string parts(matcher[5].str());
     int32_t nanoseconds= 0;

     if (parts.length() > 1)
     {
       size_t digitsCnt= parts.length() - 1;
       nanoseconds= std::stoi(parts.substr(1, std::min(digitsCnt, (size_t)9U)));

       while (digitsCnt++ < 9) {
         nanoseconds*= 10;
       }
     }

     return std::unique_ptr<Time>(new Time(matcher[0].str()));
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
 std::unique_ptr<Timestamp> TextRowProtocolCapi::getInternalTimestamp(ColumnDefinition* columnInfo, Calendar* userCalendar, TimeZone* timeZone)
 {
   std::unique_ptr<Timestamp> nullTs(new Timestamp("0000-00-00 00:00:00"));
   if (lastValueWasNull()) {
     return nullTs;
   }

   switch (columnInfo->getColumnType().getType()) {
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

     return std::unique_ptr<Timestamp>(new Timestamp(timestamp.str()));
   }
   case MYSQL_TYPE_TIME:
   {
     std::unique_ptr<Timestamp> tt(new Timestamp("1970-01-01 "));
     std::unique_ptr<Time> timeValue= getInternalTime(columnInfo, userCalendar, timeZone);

     tt->append(*timeValue);
     return tt;
   }
   default:
   {
     SQLString rawValue(fieldBuf.arr + pos, length);
     throw SQLException(
       "Value type \""
       +columnInfo->getColumnType().getTypeName()
       +"\" with value \""
       + rawValue
       +"\" cannot be parse as Timestamp");
   }
   }
 }

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
 /**
 * Get Object from raw text format.
 *
 * @param columnInfo column information
 * @param timeZone time zone
 * @return Object value
 * @throws SQLException if column type doesn't permit conversion
 */
 sql::Object* TextRowProtocolCapi::getInternalObject(ColumnDefinition* columnInfo, TimeZone* timeZone)
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
     memcpy(dataBit + 0, fieldBuf.arr + pos, length);
     return dataBit;
   case MYSQL_TYPE_TINY:
     if (options->tinyInt1isBit &&columnInfo->getLength()==1) {
       return buf[pos] !='0';
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
       memcpy(dataBit + 0, fieldBuf.arr + pos, length));
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
     memcpy(dataBit + 0, fieldBuf.arr + pos, length));
     return dataBlob;
   case MYSQL_TYPE_NULL:
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
     memcpy(dataBit + 0, fieldBuf.arr + pos, length);
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
 * Get OffsetTime format from raw text format.
 *
 * @param columnInfo column information
 * @param timeZone time zone
 * @return OffsetTime value
 * @throws SQLException if column type doesn't permit conversion
 */
 OffsetTime TextRowProtocolCapi::getInternalOffsetTime(ColumnDefinition* columnInfo, TimeZone* timeZone)
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
     SQLString raw;
     raw.reserve(fieldBuf.arr + pos, length);
     switch (columnInfo->getColumnType().getSqlType()) {
     case Types::MYSQL_TYPE_TIMESTAMP:
       if (raw.startsWith("0000-00-00 00:00:00")) {
         return nullptr;
       }
       try {
         return ZonedDateTime::parse(raw, TEXT_LOCAL_DATE_TIME.withZone(zoneOffset))
           .toOffsetDateTime()
           .toOffsetTime();
       }
       catch (DateTimeParseException& dateParserEx) {
         throw SQLException(
           raw
           +" cannot be parse as OffsetTime. time must have \"yyyy-MM-dd HH:mm:ss[.S]\" format");
       }

     case Types::MYSQL_TYPE_TIME:
       try {
         LocalTime localTime =
           LocalTime::parse(raw, DateTimeFormatter.ISO_LOCAL_TIME.withZone(zoneOffset));
         return OffsetTime.of(localTime, zoneOffset);
       }
       catch (DateTimeParseException& dateParserEx) {
         throw SQLException(
           raw
           +" cannot be parse as OffsetTime (format is \"HH:mm:ss[.S]\" for data type \""
           +columnInfo->getColumnType()
           +"\")");
       }

     case Types::MYSQL_TYPE_VARCHAR:
     case Types::LONGVARCHAR:
     case Types::CHAR:
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
 * Get LocalTime format from raw text format.
 *
 * @param columnInfo column information
 * @param timeZone time zone
 * @return LocalTime value
 * @throws SQLException if column type doesn't permit conversion
 */
 LocalTime TextRowProtocolCapi::getInternalLocalTime(ColumnDefinition* columnInfo, TimeZone* timeZone)
 {
   if (lastValueWasNull()) {
     return nullptr;
   }
   if (length == 0) {
     lastValueNull |=BIT_LAST_FIELD_NULL;
     return nullptr;
   }

   SQLString raw;
   raw.reserve(fieldBuf.arr + pos, length);

   switch (columnInfo->getColumnType().getSqlType()) {
   case Types::MYSQL_TYPE_TIME:
   case Types::MYSQL_TYPE_VARCHAR:
   case Types::LONGVARCHAR:
   case Types::CHAR:
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
 * Get LocalDate format from raw text format.
 *
 * @param columnInfo column information
 * @param timeZone time zone
 * @return LocalDate value
 * @throws SQLException if column type doesn't permit conversion
 */
 LocalDate TextRowProtocolCapi::getInternalLocalDate(ColumnDefinition* columnInfo, TimeZone* timeZone)
 {
   if (lastValueWasNull()) {
     return nullptr;
   }
   if (length == 0) {
     lastValueNull |=BIT_LAST_FIELD_NULL;
     return nullptr;
   }

   SQLString raw;
   raw.reserve(fieldBuf.arr + pos, length);

   switch (columnInfo->getColumnType().getSqlType()) {
   case Types::MYSQL_TYPE_DATE:
   case Types::MYSQL_TYPE_VARCHAR:
   case Types::LONGVARCHAR:
   case Types::CHAR:
     if (raw.startsWith("0000-00-00")) {
       return nullptr;
     }
     try {
       return LocalDate::parse(
         raw, DateTimeFormatter.ISO_LOCAL_DATE.withZone(timeZone.toZoneId()));
     }
     catch (DateTimeParseException& dateParserEx) {
       throw SQLException(
         raw
         +" cannot be parse as LocalDate (format is \"yyyy-MM-dd\" for data type \""
         +columnInfo->getColumnType()
         +"\")");
     }

   case Types::MYSQL_TYPE_TIMESTAMP:
     ZonedDateTime zonedDateTime =
       getInternalZonedDateTime(columnInfo, typeid(LocalDate), timeZone);
     return zonedDateTime/*.empty() == true*/
       ? nullptr
       : zonedDateTime.withZoneSameInstant(ZoneId.systemDefault()).toLocalDate();

   default:
     throw SQLException(
       "Cannot read LocalDate using a "
       +columnInfo->getColumnType().getCppTypeName()
       +" field");
   }
 }
#endif

 /**
  * Get int from raw text format.
  *
  * @param columnInfo column information
  * @return int value
  * @throws SQLException if column type doesn't permit conversion or not in Integer range
  */
 int32_t TextRowProtocolCapi::getInternalInt(ColumnDefinition* columnInfo)
 {
   if (lastValueWasNull()) {
     return 0;
   }
   int64_t value= getInternalLong(columnInfo);
   rangeCheck("int32_t", INT32_MIN, INT32_MAX, value, columnInfo);

   return static_cast<int32_t>(value);
 }

 /**
  * Get long from raw text format.
  *
  * @param columnInfo column information
  * @return long value
  * @throws SQLException if column type doesn't permit conversion or not in Long range (unsigned)
  */
 int64_t TextRowProtocolCapi::getInternalLong(ColumnDefinition* columnInfo)
 {
   if (lastValueWasNull()) {
     return 0;
   }

   try {
     switch (columnInfo->getColumnType().getType()) {
     case MYSQL_TYPE_FLOAT:
     case MYSQL_TYPE_DOUBLE:
     {
       long double doubleValue= std::stold(fieldBuf.arr);
       if (doubleValue > static_cast<long double>(INT64_MAX)) {
         throw SQLException(
           "Out of range value for column '"
           +columnInfo->getName()
           +"' : value "
           + SQLString(fieldBuf.arr, length)
           +" is not in int64_t range",
           "22003",
           1264);
       }
       return static_cast<int64_t>(doubleValue);
     }
     case MYSQL_TYPE_BIT:
       return parseBit();
     case MYSQL_TYPE_TINY:
     case MYSQL_TYPE_SHORT:
     case MYSQL_TYPE_YEAR:
     case MYSQL_TYPE_LONG:
     case MYSQL_TYPE_INT24:
     case MYSQL_TYPE_LONGLONG:
       return std::stoll(fieldBuf.arr);
     case MYSQL_TYPE_TIMESTAMP:
     case MYSQL_TYPE_DATETIME:
     case MYSQL_TYPE_TIME:
     case MYSQL_TYPE_DATE:
       throw SQLException(
         "Conversion to integer not available for data field type "
         + columnInfo->getColumnType().getCppTypeName());
     default:
       return std::stoll(std::string(fieldBuf.arr + pos, length));
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


 uint64_t TextRowProtocolCapi::getInternalULong(ColumnDefinition * columnInfo)
 {
   if (lastValueWasNull()) {
     return 0;
   }

   uint64_t value= 0;

   try {
     switch (columnInfo->getColumnType().getType()) {
     case MYSQL_TYPE_FLOAT:
     case MYSQL_TYPE_DOUBLE:
     {
       long double doubleValue = std::stold(fieldBuf.arr);
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
       value= sql::mariadb::stoull(fieldBuf.arr);
       break;
     case MYSQL_TYPE_TIMESTAMP:
     case MYSQL_TYPE_DATETIME:
     case MYSQL_TYPE_TIME:
     case MYSQL_TYPE_DATE:
       throw SQLException(
         "Conversion to integer not available for data field type "
         + columnInfo->getColumnType().getCppTypeName());
     default:
       value= sql::mariadb::stoull(fieldBuf.arr + pos, length);
     }

   }
   // Common parent for std::invalid_argument and std::out_of_range
   catch (std::logic_error&) {
     /*std::stoll and std::stoull take care of */
     /*std::string value(fieldBuf.arr + pos, length);
     if (std::regex_match(value, isIntegerRegex)) {
       try {
         return std::stoull(value.substr(0, value.find_first_of('.')));
       }
       catch (std::exception&) {

       }
     }*/
     throw SQLException(
       "Out of range value for column '" + columnInfo->getName() + "' : value " + value,
       "22003",
       1264);
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
 float TextRowProtocolCapi::getInternalFloat(ColumnDefinition* columnInfo)
 {
   if (lastValueWasNull()) {
     return 0;
   }

   switch (columnInfo->getColumnType().getType()) {
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
           +SQLString(fieldBuf.arr + pos, length)
           +"\" for getFloat for data field with type "
           +columnInfo->getColumnType().getCppTypeName(),
           "22003",
           1264, &nfe);
     }
   default:
     throw SQLException(
       "getFloat not available for data field type "
       +columnInfo->getColumnType().getCppTypeName());
   }
 }

 /**
  * Get double from raw text format.
  *
  * @param columnInfo column information
  * @return double value
  * @throws SQLException if column type doesn't permit conversion or not in Double range (unsigned)
  */
 long double TextRowProtocolCapi::getInternalDouble(ColumnDefinition* columnInfo)
 {
   if (lastValueWasNull()) {
     return 0;
   }
   switch (columnInfo->getColumnType().getType()) {
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
       return stringToDouble(fieldBuf.arr + pos, length);
     }
     // Common parent for std::invalid_argument and std::out_of_range
     catch (std::logic_error& nfe) {
       throw SQLException(
           "Incorrect format \""
           + SQLString(fieldBuf.arr + pos, length)
           +"\" for getDouble for data field with type "
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
  * Get BigDecimal from raw text format.
  *
  * @param columnInfo column information
  * @return BigDecimal value
  */
 std::unique_ptr<BigDecimal> TextRowProtocolCapi::getInternalBigDecimal(ColumnDefinition* columnInfo)
 {
   if (lastValueWasNull()) {
     return std::unique_ptr<BigDecimal>();
   }

   if (columnInfo->getColumnType()==ColumnType::BIT) {
     return std::unique_ptr<BigDecimal>(new BigDecimal(std::to_string(parseBit())));
   }
   return std::unique_ptr<BigDecimal>(new BigDecimal(fieldBuf.arr + pos, length));
 }


  /**
  * Get boolean from raw text format.
  *
  * @param columnInfo column information
  * @return boolean value
  */
 bool TextRowProtocolCapi::getInternalBoolean(ColumnDefinition* columnInfo)
 {
   if (lastValueWasNull()) {
     return false;
   }

   if (columnInfo->getColumnType()==ColumnType::BIT) {
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
 int8_t TextRowProtocolCapi::getInternalByte(ColumnDefinition* columnInfo)
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
 int16_t TextRowProtocolCapi::getInternalShort(ColumnDefinition* columnInfo)
 {
   if (lastValueWasNull()) {
     return 0;
   }
   int64_t value= getInternalLong(columnInfo);
   rangeCheck("int16_t", INT16_MIN, INT16_MAX, value, columnInfo);
   return static_cast<int16_t>(value);
 }

 /**
  * Get Time in string format from raw text format.
  *
  * @param columnInfo column information
  * @return String representation of time
  */
 SQLString TextRowProtocolCapi::getInternalTimeString(ColumnDefinition* columnInfo)
 {
   if (lastValueWasNull()) {
     return "";
   }

   SQLString rawValue(fieldBuf.arr + pos, length);
   if (rawValue.compare("0000-00-00") == 0) {
     return "";
   }

   if (options->maximizeMysqlCompatibility
     && rawValue.find_first_of('.') != std::string::npos) {
     return rawValue.substr(0, rawValue.find_first_of('.'));
   }
   return rawValue;
 }


 int32_t TextRowProtocolCapi::fetchNext()
 {
   //Assuming it is called only for the case of the data from server, and not constructed text results
   rowData= mysql_fetch_row(capiResults.get());
   lengthArr= mysql_fetch_lengths(capiResults.get());

   return (rowData == nullptr ? MYSQL_NO_DATA : 0);
 }


 void TextRowProtocolCapi::installCursorAtPosition(int32_t rowPtr)
 {
   mysql_data_seek(capiResults.get(), static_cast<unsigned long long>(rowPtr));
 }

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
 /**
 * Get BigInteger format from raw text format.
 *
 * @param columnInfo column information
 * @return BigInteger value
 */
 BigInteger TextRowProtocolCapi::getInternalBigInteger(ColumnDefinition* columnInfo)
 {
   if (lastValueWasNull()) {
     return nullptr;
   }
   return new BigInteger(new SQLString(fieldBuf.arr + pos, length));
 }

 /**
  * Get ZonedDateTime format from raw text format.
  *
  * @param columnInfo column information
  * @param clazz class for logging
  * @param timeZone time zone
  * @return ZonedDateTime value
  * @throws SQLException if column type doesn't permit conversion
  */
 ZonedDateTime TextRowProtocolCapi::getInternalZonedDateTime(ColumnDefinition* columnInfo, Class clazz, TimeZone* timeZone)
 {
   if (lastValueWasNull()) {
     return nullptr;
   }
   if (length == 0) {
     lastValueNull |=BIT_LAST_FIELD_NULL;
     return nullptr;
   }

   SQLString raw;
   raw.reserve(fieldBuf.arr + pos, length);

   switch (columnInfo->getColumnType().getSqlType()) {
   case Types::MYSQL_TYPE_TIMESTAMP:
     if (raw.startsWith("0000-00-00 00:00:00")) {
       return nullptr;
     }
     try {
       LocalDateTime localDateTime =
         LocalDateTime::parse(raw, TEXT_LOCAL_DATE_TIME.withZone(timeZone.toZoneId()));
       return ZonedDateTime.of(localDateTime, timeZone.toZoneId());
     }
     catch (DateTimeParseException& dateParserEx) {
       throw SQLException(
         raw
         +" cannot be parse as LocalDateTime. time must have \"yyyy-MM-dd HH:mm:ss[.S]\" format");
     }

   case Types::MYSQL_TYPE_VARCHAR:
   case Types::LONGVARCHAR:
   case Types::CHAR:
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
#endif

 /**
  * Indicate if data is binary encoded.
  *
  * @return always false.
  */
 bool TextRowProtocolCapi::isBinaryEncoded()
 {
   return false;
 }

}
}
}
