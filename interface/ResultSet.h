/************************************************************************************
   Copyright (C) 2022 MariaDB Corporation AB

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


#ifndef _ResultSet_H_
#define _ResultSet_H_

#include <exception>
#include <vector>

#include "mysql.h"

#include "CArray.h"
#include "Row.h"
#include "ColumnDefinition.h"
#include "pimpls.h"

namespace mariadb
{

extern const MYSQL_FIELD FIELDBIGINT;
extern const MYSQL_FIELD FIELDSTRING;
extern const MYSQL_FIELD FIELDSHORT;
extern const MYSQL_FIELD FIELDINT;

class ResultSet
{
  void operator=(ResultSet&)= delete;

protected:
  static uint64_t MAX_ARRAY_SIZE;

  static int32_t TINYINT1_IS_BIT; /*1*/
  static int32_t YEAR_IS_DATE_TYPE; /*2*/

  Protocol*           protocol=      nullptr;
  int32_t             dataFetchTime= 0;
  bool                streaming=     false;
  int32_t             fetchSize=     0;
  mutable Row*        row= nullptr;
  bool                isEof=         false;
  std::vector<ColumnDefinition> columnsInformation;
  int32_t         columnInformationLength= 0;
  int32_t         rowPointer=             -1;
  mutable int32_t lastRowPointer=         -1;
  std::vector<std::vector<mariadb::bytes_view>> data;
  std::size_t dataSize=           0; //Should go after data
  bool        noBackslashEscapes= false;
  // we don't create buffers for all columns without call. Thus has to be mutable while getters are const
  mutable std::map<int32_t, std::unique_ptr<memBuf>> blobBuffer;
  int32_t resultSetScrollType= TYPE_SCROLL_INSENSITIVE;
  //  std::unique_ptr<ColumnNameMap> columnNameMap;
  bool isClosedFlag= false;
  bool forceAlias=   false;

  PreparedStatement* statement= nullptr;

  ResultSet(Protocol* guard, Results* results);

  ResultSet(Protocol* guard, std::vector<ColumnDefinition>& columnInformation,
    const std::vector<std::vector<mariadb::bytes_view>>& resultSet,
    int32_t rsScrollType=TYPE_SCROLL_INSENSITIVE);

  ResultSet(Protocol* guard, Results* results,
    const std::vector<ColumnDefinition>& columnInformation);

  ResultSet(Protocol * _protocol, const MYSQL_FIELD* field,
    std::vector<std::vector<mariadb::bytes_view>>& resultSet, int32_t rsScrollType);

public:

  enum {
    TYPE_FORWARD_ONLY= 0,
    TYPE_SCROLL_INSENSITIVE, // Static cursor
    TYPE_SCROLL_SENSITIVE    // Dynamic cursor
  };

  static ResultSet* create(
    Results*,
    Protocol* _protocol,
    ServerPrepareResult* pr,
    bool callableResult= false);

  static ResultSet* create(
    Results*,
    Protocol* _protocol,
    MYSQL* capiConnHandle);

  static ResultSet* create(
    const MYSQL_FIELD* columnInformation,
    std::vector<std::vector<bytes_view>>& resultSet,
    Protocol* _protocol,
    int32_t resultSetScrollType);

  static ResultSet* create(
    std::vector<ColumnDefinition>& columnInformation,
    const std::vector<std::vector<bytes_view>>& resultSet,
    Protocol* _protocol,
    int32_t resultSetScrollType);

  static ResultSet* createGeneratedData(std::vector<int64_t>& data, bool findColumnReturnsOne);
  static ResultSet* createEmptyResultSet();

  /**
  * Create a result set from given data. Useful for creating "fake" resultSets for
  * DatabaseMetaData, (one example is MariaDbDatabaseMetaData.getTypeInfo())
  *
  * @param columnNames - string array of column names
  * @param columnTypes - column types
  * @param data - each element of this array represents a complete row in the ResultSet. Each value
  *     is given in its string representation, as in MariaDB text protocol, except boolean (BIT(1))
  *     values that are represented as "1" or "0" strings
  * @param protocol protocol
  * @return resultset
  */

  static ResultSet* createResultSet(const std::vector<SQLString>& columnNames, const std::vector<const MYSQL_FIELD*>& columnTypes,
    const std::vector<std::vector<bytes_view>>& data);

  virtual ~ResultSet();

  void close();
  bool next();

  virtual bool isFullyLoaded() const=0;
  // Fetches remaining result set from the server
  virtual void fetchRemaining()=0;
  // Caches the whole resultset locally, i.e. on c/c++ side unlike "fetch" functions that mean fetching from server and storing on C/C side
  // In text protocol it may be more or less the same(unless result is streamed), while in binary - 2 different things.
  virtual void cacheCompleteLocally()=0;
  virtual ResultSetMetaData* getMetaData() const=0;
  virtual std::size_t rowsCount() const=0;

  virtual bool isLast()=0;
  virtual bool isAfterLast()=0;

  virtual void beforeFirst()=0;
  virtual void afterLast()=0;
  virtual bool first()=0;
  virtual bool last()=0;
  virtual int64_t getRow()=0;
  virtual bool absolute(int64_t row)=0;
  virtual bool relative(int64_t rows)=0;
  virtual bool previous()=0;
 
protected:
  void resetVariables();
  void handleIoException(std::exception& ioe) const;
  /* Some classes(Results) may hold pointer to this object - it may be required in case of RS streaming to
     to fetch remaining rows in order to unblock connection for new queries, or to close RS, if next RS is requested or statements is destructed.
     After releasing the RS by API methods, it's owned by application. If app destructs the RS, this method is called by destructor, and
     implementation should do the job on checking out of the object, so it can't be attempted to use any more */
  void checkOut();

  virtual std::vector<bytes_view>& getCurrentRowData()=0;
  virtual void updateRowData(std::vector<bytes_view>& rawData)=0;
  virtual void deleteCurrentRowData()=0;
  virtual void addRowData(std::vector<bytes_view>& rawData)=0;
          void addStreamingValue(bool cacheLocally= false);
  virtual bool readNextValue(bool cacheLocally= false)=0;
  virtual void setRowPointer(int32_t pointer)=0;
          bool fillBuffers(MYSQL_BIND* resBind);
          bool getCached(MYSQL_BIND* bind, uint32_t column0basedIdx, uint64_t offset);
          void nextStreamingValue();

  // Keeping them private so far
  virtual std::istream* getBinaryStream(int32_t columnIndex) const=0;
  virtual int32_t getInt(int32_t columnIndex) const=0;
  virtual int64_t getLong(int32_t columnIndex) const=0;
  virtual uint64_t getUInt64(int32_t columnIndex) const=0;
  virtual uint32_t getUInt(int32_t columnIndex) const=0;
  virtual float getFloat(int32_t columnIndex) const=0;
  virtual long double getDouble(int32_t columnIndex) const=0;
  virtual bool getBoolean(int32_t index) const=0;
  virtual int8_t getByte(int32_t index) const=0;
  virtual int16_t getShort(int32_t index) const=0;

  bool isNull(int32_t columnIndex) const;
  void resetRow() const;
  void checkObjectRange(int32_t position) const;
public:
  virtual void abort()=0;
  virtual bool isCallableResult() const=0;
//  virtual PreparedStatement* getStatement()=0;
  virtual void setForceTableAlias()=0;
  virtual int32_t getRowPointer()=0;
  
  virtual void bind(MYSQL_BIND* result)= 0;
  /* Fetches single column value into BIND structure. Returns true on error */
  virtual bool get(MYSQL_BIND* result, uint32_t column0basedIdx, uint64_t offset)= 0;
  /* Fills all bound buffers */
  virtual bool get()= 0;
  virtual bool setResultCallback(ResultCodec* callback, uint32_t column= uint32_t(-1))= 0;
  virtual bool setCallbackData(void* data)= 0;
  virtual std::size_t getDataSize()=0;
  virtual bool isBinaryEncoded()=0;
};

} // namespace mariadb
#endif
