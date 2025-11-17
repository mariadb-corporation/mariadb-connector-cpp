/************************************************************************************
   Copyright (C) 2022, 2025 MariaDB Corporation AB

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


#ifndef _SERVERSIDEPREPAREDSTATEMENT_H_
#define _SERVERSIDEPREPAREDSTATEMENT_H_

#include "ServerPrepareResult.h"
#include "PreparedStatement.h"


namespace mariadb
{

extern "C"
{
  // C/C's callback running param column callbacks 
  my_bool* defaultParamCallback(void* data, MYSQL_BIND* bind, uint32_t row_nr);
  // C/C's callback running callback for the row, and then indivivual param column callbacks
  my_bool* withRowCheckCallback(void* data, MYSQL_BIND* bind, uint32_t row_nr);
}

class ServerSidePreparedStatement : public PreparedStatement
{
  friend my_bool* withRowCheckCallback(void* data, MYSQL_BIND* bind, uint32_t row_nr);
  friend my_bool* defaultParamCallback(void* data, MYSQL_BIND* binds, uint32_t row_nr);

protected:
  ServerPrepareResult* serverPrepareResult= nullptr;

public:
  ~ServerSidePreparedStatement();
  ServerSidePreparedStatement(Protocol* connection, const SQLString& sql, int32_t resultSetScrollType);
  ServerSidePreparedStatement(Protocol* connection, ServerPrepareResult* pr, int32_t resultSetScrollType);

  ServerSidePreparedStatement* clone(Protocol* connection);

private:
  /* For cloning */
  ServerSidePreparedStatement(
    Protocol* connection,
    int32_t resultSetScrollType
    );

  void prepare(const SQLString& sql);
  void setMetaFromResult();
  virtual void executeBatchInternal(uint32_t queryParameterSize);

public:
  //void setParameter(int32_t parameterIndex,/*const*/ ParameterHolder* holder);
  //ParameterMetaData* getParameterMetaData();
  ResultSetMetaData* getMetaData();

protected:
  void executeQueryPrologue(ServerPrepareResult* serverPrepareResult);
  void getResult();

public:
  PrepareResult* getPrepareResult() override { return dynamic_cast<PrepareResult*>(serverPrepareResult); }
  bool executeInternal(int32_t fetchSize) override;

  void close();

  const char* getError() override;
  uint32_t    getErrno() override;
  const char* getSqlState() override;

  bool        bind(MYSQL_BIND* param) override;
  bool        sendLongData(uint32_t paramNum, const char* data, std::size_t length) override;
  inline bool isServerSide() const override { return true; }
  enum enum_field_types getPreferredParamType(enum enum_field_types appType) const override
  {
    return appType;
  }

  bool setParamCallback(ParamCodec* callback, uint32_t param= uint32_t(-1)) override;
  bool setCallbackData(void* data) override;
  bool isOutParams() override;
  };
}
#endif
