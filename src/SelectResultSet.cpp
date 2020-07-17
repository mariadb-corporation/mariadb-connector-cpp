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


#include <vector>

#include "SelectResultSet.h"
#include "Results.h"
#include "Protocol.h"
#include "ColumnDefinition.h"
#include "util/ServerPrepareResult.h"

#include "com/capi/SelectResultSetCapi.h"
#include "com/capi/ColumnDefinitionCapi.h"

namespace sql
{
namespace mariadb
{

  int32_t SelectResultSet::TINYINT1_IS_BIT= 1;
  int32_t SelectResultSet::YEAR_IS_DATE_TYPE= 2;
  const SQLString SelectResultSet::NOT_UPDATABLE_ERROR("Updates are not supported when using ResultSet::CONCUR_READ_ONLY");

  std::vector<Shared::ColumnDefinition> SelectResultSet::INSERT_ID_COLUMNS;

  int32_t SelectResultSet::MAX_ARRAY_SIZE= INT32_MAX - 8;

  /**
    * Create Streaming resultSet.
    *
    * @param columnInformation column information
    * @param results results
    * @param protocol current protocol
    * @param reader stream fetcher
    * @param callableResult is it from a callableStatement ?
    * @param eofDeprecated is EOF deprecated
    * @throws IOException if any connection error occur
    * @throws SQLException if any connection error occur
    */
  SelectResultSet* SelectResultSet::create(
    std::vector<Shared::ColumnDefinition>& columnInformation,
    Results* results,
    Protocol* protocol,
    PacketInputStream* reader,
    bool callableResult,
    bool eofDeprecated)
  {
    return nullptr;// new SelectResultSetPacket(columnInformation, results, protocol, reader, callableResult, eofDeprecated);
  }

  SelectResultSet* SelectResultSet::create(Results * results, Protocol * protocol, ServerPrepareResult * spr, bool callableResult, bool eofDeprecated)
  {
    return new capi::SelectResultSetCapi(results, protocol, spr, callableResult, eofDeprecated);
  }


  SelectResultSet* SelectResultSet::create(Results* results,
                                           Protocol* protocol,
                                           capi::MYSQL* connection,
                                           bool eofDeprecated)
  {
    return new capi::SelectResultSetCapi(results, protocol, connection, eofDeprecated);
  }

  /**
    * Create filled result-set.
    *
    * @param columnInformation column information
    * @param resultSet result-set data
    * @param protocol current protocol
    * @param resultSetScrollType one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.TYPE_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
    *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    */
  SelectResultSet* SelectResultSet::create(
    std::vector<Shared::ColumnDefinition>& columnInformation,
    /*std::unique_ptr<*/std::vector<std::vector<sql::bytes>>& resultSet,
    Protocol* protocol,
    int32_t resultSetScrollType)
  {
    return new capi::SelectResultSetCapi(columnInformation, resultSet, protocol, resultSetScrollType);
  }

  /**
    * Create a result set from given data. Useful for creating "fake" resultsets for
    * DatabaseMetaData, (one example is MariaDbDatabaseMetaData.getTypeInfo())
    *
    * @param data - each element of this array represents a complete row in the ResultSet. Each value
    *     is given in its string representation, as in MariaDB text protocol, except boolean (BIT(1))
    *     values that are represented as "1" or "0" strings
    * @param protocol protocol
    * @param findColumnReturnsOne - special parameter, used only in generated key result sets
    * @return resultset
    */
  ResultSet* SelectResultSet::createGeneratedData(std::vector<int64_t>& data, Protocol* protocol, bool findColumnReturnsOne)
  {
    std::vector<Shared::ColumnDefinition> columns{capi::ColumnDefinitionCapi::create("insert_id", ColumnType::BIGINT)};
    std::vector<std::vector<sql::bytes>> rows;
    std::string idAsStr;


    for (int64_t rowData : data) {
      std::vector<sql::bytes> row;
      if (rowData != 0) {
        idAsStr= std::to_string(rowData);
        BYTES_ASSIGN_STR(row[0], idAsStr);
        rows.push_back(row);
      }
    }
    if (findColumnReturnsOne) {
      return create(columns, rows, protocol, TYPE_SCROLL_SENSITIVE);
      /*int32_t SelectResultSet::findColumn(const SQLString& name) {
        return 1;
      }*/
    }

    return new capi::SelectResultSetCapi(columns, rows, protocol, TYPE_SCROLL_SENSITIVE);
  }

  bool SelectResultSet::InitIdColumns() {
    SelectResultSet::INSERT_ID_COLUMNS.push_back(capi::ColumnDefinitionCapi::create("insert_id", ColumnType::BIGINT));
    return true;
  }

  SelectResultSet* SelectResultSet::createEmptyResultSet() {
    static bool inited= InitIdColumns();
    static std::vector<std::vector<sql::bytes>> emptyRs;

    return create(INSERT_ID_COLUMNS, emptyRs, nullptr, TYPE_SCROLL_SENSITIVE);
  }

  ResultSet * SelectResultSet::createResultSet(std::vector<SQLString>& columnNames,
    std::vector<ColumnType>& columnTypes,
    std::vector<std::vector<sql::bytes>>& data,
    Protocol* protocol)
  {
    std::size_t columnNameLength= columnNames.size();

    std::vector<Shared::ColumnDefinition> columns;
    columns.reserve(columnTypes.size());

    for (std::size_t i= 0; i <columnNameLength; i++) {
      columns.emplace_back(ColumnDefinition::create(columnNames[i], columnTypes[i]));
    }

    /*std::vector<sql::bytes> rows;

    for (auto& rowData : data) {
      //assert(rowData.size() == columnNameLength);
      //char* rowBytes[]= new char[rowData.size()][];
      //for (size_t i= 0; i <rowData.size(); i++) {
      //if (rowData[i].empty() != true) {
      //memcpy(rowBytes[i], rowData[i].c_str(), rowData[i].length());
      //}
      //}
      rows.push_back(StandardPacketInputStream::create<COLUMNSCOUNT>(rowData, columnTypes));
    }*/
    return create(columns, data, protocol, TYPE_SCROLL_SENSITIVE);
  }

}
}
