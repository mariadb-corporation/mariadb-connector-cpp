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

#include "mysql.h"

#include "ServerPrepareResult.h"
#include "ResultSetMetaData.h"
#include "Exception.h"

#include "ColumnDefinition.h"
#include "Protocol.h"

namespace mariadb
{
  static constexpr my_bool myboolTrue= 1;

  ServerPrepareResult::~ServerPrepareResult()
  {
    if (statementId) {
      connection->forceReleasePrepareStatement(statementId);
    }
  }
  /**
    * PrepareStatement Result object.
    *
    * @param sql query
    * @param unProxiedProtocol indicate the protocol on which the prepare has been done
    */
  ServerPrepareResult::ServerPrepareResult(
    const SQLString& _sql,
    Protocol* guard
    )
    : sql(_sql)
    , connection(guard)
    , statementId(mysql_stmt_init(guard->getCHandle()))
  {
    int rc= 1;

    if (statementId == nullptr) {
      throw rc;
    }
    mysql_stmt_attr_set(statementId, STMT_ATTR_UPDATE_MAX_LENGTH, &myboolTrue);

    if ((rc= mysql_stmt_prepare(statementId, sql.c_str(), static_cast<unsigned long>(sql.length()))) != 0) {
      SQLException e(mysql_stmt_error(statementId), mysql_stmt_sqlstate(statementId), mysql_stmt_errno(statementId));
      mysql_stmt_close(statementId);
      throw e;
    }
    paramCount= mysql_stmt_param_count(statementId);
    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> metadata(mysql_stmt_result_metadata(statementId), &mysql_free_result);
    if (metadata) {
      init(mysql_fetch_fields(metadata.get()), mysql_stmt_field_count(statementId));
    }
  }

  ServerPrepareResult::ServerPrepareResult(const SQLString& _sql, unsigned long _paramCount, Protocol* guard)
    : sql(_sql)
    , connection(guard)
    , statementId(mysql_stmt_init(guard->getCHandle()))
    , paramCount(_paramCount)
  {
    mysql_stmt_attr_set(statementId, STMT_ATTR_UPDATE_MAX_LENGTH, &myboolTrue);
    unsigned int pCount= static_cast<unsigned int>(paramCount); // TODO: consider to change paramCount to uint
    mysql_stmt_attr_set(statementId, STMT_ATTR_PREBIND_PARAMS, &pCount);
  }

  /**
  * PrepareStatement Result object.
  *
  * @param sql query
  * @param statementId server statement Id.
  * @param columns columns information
  * @param parameters parameters information
  * @param unProxiedProtocol indicate the protocol on which the prepare has been done
  */
  ServerPrepareResult::ServerPrepareResult(
    const SQLString& _sql,
    MYSQL_STMT* _statementId,
    Protocol* guard)
    : sql(_sql)
    , statementId(_statementId)
    , connection (guard)
    , paramCount(mysql_stmt_param_count(statementId))
  {
    // Feels like this can be done better
    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> metadata(mysql_stmt_result_metadata(statementId), &mysql_free_result);
    if (metadata) {
      init(mysql_fetch_fields(metadata.get()), mysql_stmt_field_count(statementId));
    }
  }


  void ServerPrepareResult::reReadColumnInfo()
  {
    std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> metadata(mysql_stmt_result_metadata(statementId), &mysql_free_result);
    column.clear();
    field.clear();
    init(mysql_fetch_fields(metadata.get()), mysql_stmt_field_count(statementId));
  }

  //void ServerPrepareResult::resetParameterTypeHeader()
  //{
  //  paramBind.clear();

  //  if (paramCount > 0) {
  //    paramBind.resize(paramCount);
  //  }
  //}

  size_t ServerPrepareResult::getParamCount() const
  {
    return paramCount;
  }


  MYSQL_STMT* ServerPrepareResult::getStatementId()
  {
    return statementId;
  }

  const MYSQL_FIELD* ServerPrepareResult::getFields() const
  {
    return *field.data();//reinterpret_cast<const MYSQL_FIELD*>(field.data());
  }

  const std::vector<ColumnDefinition>& ServerPrepareResult::getColumns() const
  {
    return column;
  }

  const SQLString& ServerPrepareResult::getSql() const
  {
    return sql;
  }


  ResultSetMetaData* ServerPrepareResult::getEarlyMetaData()
  {
    return new ResultSetMetaData(column);
  }


  /**
    * Increment share counter.
    *
    * @return true if can be used (is not been deallocate).
    */
  bool ServerPrepareResult::incrementShareCounter()
  {
    std::lock_guard<std::mutex> localScopeLock(lock);
    if (isBeingDeallocate) {
      return false;
    }
    ++shareCounter;
    return true;
  }

  void ServerPrepareResult::decrementShareCounter()
  {
    std::lock_guard<std::mutex> localScopeLock(lock);
    --shareCounter;
  }

  /**
    * Asked if can be deallocate (is not shared in other statement and not in cache) Set deallocate
    * flag to true if so.
    *
    * @return true if can be deallocate
    */
  bool ServerPrepareResult::canBeDeallocate()
  {
    std::lock_guard<std::mutex> localScopeLock(lock);

    if (shareCounter > 1 || isBeingDeallocate) {
      return false;
    }
    isBeingDeallocate= true;
    return true;
  }


  // for unit test
  int32_t ServerPrepareResult::getShareCounter()
  {
    std::lock_guard<std::mutex> localScopeLock(lock);
    return static_cast<int32_t>(shareCounter);
  }

  /*void initBindStruct(MYSQL_BIND& bind, const MYSQL_BIND* paramInfo)
  {
    std::memset(&bind, 0, sizeof(bind));

    bind.buffer_type= static_cast<enum_field_types>(typeInfo.getType());
    bind.is_null= &bind.is_null_value;
    if (paramInfo.isUnsigned()) {
      bind.is_unsigned= '\1';
    }
  }*/

  /*void bindParamValue(MYSQL_BIND& bind)
  {
    bind.is_null_value= '\0';
    bind.long_data_used= '\0';

    if (param->isNullData()) {
      bind.is_null_value= '\1';
      return;
    }

    if (param->isLongData()) {
      bind.long_data_used= '\1';
      return;
    }

    if (param->isUnsigned()) {
      bind.is_unsigned= '\1';
    }

    bind.buffer= param->getValuePtr();
    bind.buffer_length=param->getValueBinLen();
  }*/

  //void ServerPrepareResult::bindParameters(std::vector<Unique::ParameterHolder>& paramValue)
  //{
  //  for (std::size_t i= 0; i < paramCount; ++i)
  //  {
  //    auto& bind= paramBind[i];

  //    initBindStruct(bind, *paramValue[i]);
  //    bindParamValue(bind, paramValue[i]);
  //  }
  //  mysql_stmt_bind_param(statementId, paramBind.data());
  //}


  //void ServerPrepareResult::bindParameters(MYSQL_BIND* paramValue, const int16_t* type)
  //{
  //  uint32_t i= 0;
  //  for (auto& bind : paramBind)
  //  {
  //    // Initing with first row param data
  //    initBindStruct(bind, paramValue[i]);
  //    if (type != nullptr) {
  //      bind.buffer_type= static_cast<enum_field_types>(type[i]);
  //    }
  //    ++i;
  //  }
  //  
  //  mysql_stmt_attr_set(statementId, STMT_ATTR_CB_USER_DATA, &paramValue);
  //  mysql_stmt_attr_set(statementId, STMT_ATTR_CB_PARAM, (const void*)&paramRowUpdateCallback);
  //  mysql_stmt_bind_param(statementId, paramBind);
  //}

} // namespace mariadb
