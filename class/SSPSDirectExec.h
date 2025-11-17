/************************************************************************************
   Copyright (C) 2025 MariaDB Corporation plc

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


#ifndef _SSPSDIRECTECEC_H_
#define _SSPSDIRECTECEC_H_

#include "ServerSidePreparedStatement.h"


namespace mariadb
{

class SSPSDirectExec : public ServerSidePreparedStatement
{
public:
  ~SSPSDirectExec();
  SSPSDirectExec(Protocol* connection, const SQLString& sql, unsigned long _paramCount, int32_t resultSetScrollType);

  //SSPSDirectExec* clone(Protocol* connection);

private:
  SSPSDirectExec(
    Protocol* connection,
    int32_t resultSetScrollType
    );


public:
  //void setParameter(int32_t parameterIndex,/*const*/ ParameterHolder* holder);
  //ParameterMetaData* getParameterMetaData();
  ResultSetMetaData* getMetaData();

private:
  void executeBatchInternal(uint32_t queryParameterSize) override;

public:
  //PrepareResult* getPrepareResult() { return dynamic_cast<PrepareResult*>(serverPrepareResult); }
  bool executeInternal(int32_t fetchSize);

  void close();

  /*const char* getError();
  uint32_t    getErrno();
  const char* getSqlState();

  bool bind(MYSQL_BIND* param);
  enum enum_field_types getPreferredParamType(enum enum_field_types appType) const
  {
    return appType;
  }

  bool setParamCallback(ParamCodec* callback, uint32_t param= uint32_t(-1));
  bool setCallbackData(void* data);
  bool isOutParams() override;*/
  };
}
#endif
