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


#ifndef _PREPARESTATEMENT_H_
#define _PREPARESTATEMENT_H_

#include <map>
#include "PrepareResult.h"
#include "CArray.h"
#include "pimpls.h"
#include "class/ResultSetMetaData.h"
namespace mariadb
{
  extern char paramIndicatorNone, paramIndicatorNull, paramIndicatorDef, paramIndicatorNts, paramIndicatorIgnore,
    paramIndicatorNull, paramIndicatorIgnoreRow;
// The functor returns bool only to fit the need to check if to skip paramset, and I don't really need to introduce 2 diff callbacks for that, so one "universal"
class ParamCodec
{
public:
  virtual ~ParamCodec() {}
  virtual bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr)= 0;
};

template <typename T>
class PCodecCallable : public ParamCodec
{
public:
  virtual ~PCodecCallable() {}
  virtual bool operator()(void *data, MYSQL_BIND *bind, uint32_t col_nr, uint32_t row_nr)
  {
    return T(data, bind, col_nr, row_nr);
  }
};


class ResultCodec
{
public:
  virtual ~ResultCodec() {}
  virtual void operator()(void *data, uint32_t col_nr, unsigned char *row, unsigned long length)= 0;
};


template <typename T>
class RCodecCallable : public ResultCodec
{
public:
  virtual ~RCodecCallable() {}
  virtual void operator()(void *data, uint32_t col_nr, unsigned char *row, unsigned long length) { T(data, col_nr, row); }
};


class PreparedStatement
{
protected:
  Protocol* guard;

  //PrepareResult owns query string
  const SQLString* sql= nullptr;
  std::size_t      parameterCount= 0; // Do we need it here if we can get it from prepare result class
  bool hasLongData= false;
  bool useFractionalSeconds= true;
  int32_t fetchSize= 0;
  int32_t resultSetScrollType= 0;
  bool  closed= false;
  Longs batchRes;
  Unique::ResultSetMetaData metadata;
  Unique::Results results;
  MYSQL_BIND*     param= nullptr;
  uint32_t        batchArraySize= 0;
  bool     continueBatchOnError= false;
  uint32_t queryTimeout= 0;
  std::map<std::size_t, ParamCodec*> parColCodec;
  ParamCodec* parRowCallback= nullptr;
  void* callbackData= nullptr;
  
  PreparedStatement(
    Protocol* handle,
    int32_t resultSetScrollType);
  PreparedStatement(
    Protocol* handle,
    const SQLString& _sql,
    int32_t resultSetScrollType);

public:

  enum {
    EXECUTE_FAILED= -3,
    SUCCESS_NO_INFO= -2
  };

  virtual ~PreparedStatement();

protected:
  virtual bool executeInternal(int32_t fetchSize)=0;
  virtual PrepareResult* getPrepareResult()=0;
  virtual void executeBatchInternal(uint32_t queryParameterSize)=0;

  /**
   * Check if statement is closed, and throw exception if so.
   *
   * @throws SQLException if statement close
   */
  void checkClose();
  void markClosed();
  void validateParamset(std::size_t paramCount);

public:
  int64_t    getServerThreadId();
  inline
   Protocol* getProtocol() { return guard; }
  void       clearBatch();
  int64_t    executeUpdate();
  bool       execute();
  ResultSet* executeQuery();
  ResultSet* getResultSet();
  int64_t    getUpdateCount();

  const Longs&        executeBatch();
  ResultSetMetaData*  getEarlyMetaData();
  std::size_t         getParamCount();

  void setBatchSize(int32_t batchSize);
  bool getMoreResults();
  bool hasMoreResults();

  virtual const char* getError()=0;
  virtual uint32_t    getErrno()=0;
  virtual const char* getSqlState()= 0;;

  virtual bool bind(MYSQL_BIND* param)=0;
  virtual bool sendLongData(uint32_t paramNum, const char* data, std::size_t length)=0;
  virtual bool isServerSide() const=0;
  virtual enum enum_field_types getPreferredParamType(enum enum_field_types appType) const=0;
  Results*    getInternalResults() { return results.get(); }
  inline void setFetchSize(int32_t _fetchSize) { fetchSize= _fetchSize; }
  inline int32_t getFetchSize() { return fetchSize; }
  // Return false if callbacks are not supported
  virtual bool setParamCallback(ParamCodec* callback, uint32_t param= uint32_t(-1))= 0;
  virtual bool setCallbackData(void* data)= 0;
  // Returns if current result is out params
  virtual bool isOutParams()= 0;
  };

} // namespace mariadb
#endif
