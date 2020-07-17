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


#include "CallableParameterMetaData.h"

#include "CallParameter.h"
#include "ColumnType.h"
#include "MariaDbConnection.h"

namespace sql
{
namespace mariadb
{

  std::regex CallableParameterMetaData::PARAMETER_PATTERN("\\s*(IN\\s+|OUT\\s+|INOUT\\s+)?([\\w\\d]+)\\s+(UNSIGNED\\s+)?(\\w+)\\s*(\\([\\d,]+\\))?\\s*",
    std::regex_constants::ECMAScript | std::regex_constants::icase);

  std::regex CallableParameterMetaData::RETURN_PATTERN("\\s*(UNSIGNED\\s+)?(\\w+)\\s*(\\([\\d,]+\\))?\\s*(CHARSET\\s+)?(\\w+)?\\s*",
    std::regex_constants::ECMAScript | std::regex_constants::icase);
  /**
    * Retrieve Callable metaData.
    *
    * @param con connection
    * @param database database name
    * @param name procedure/function name
    * @param isFunction is it a function
    */
  CallableParameterMetaData::CallableParameterMetaData(MariaDbConnection* conn, const SQLString& _database, const SQLString& _name, bool _isFunction)
    :  con(conn)
    , database(_database)
    , name(_name)
    , isFunction(_isFunction)
  {
    if (!database.empty()) {
      replace(database, "`", "");
    }
    replace(name, "`", "");
  }

  /**
    * Search metaData if not already loaded.
    *
    * @throws SQLException if error append during loading metaData
    */
  void CallableParameterMetaData::readMetadataFromDbIfRequired()
  {
    if (valid) {
      return;
    }
    readMetadata();
    valid= true;
  }

  int32_t CallableParameterMetaData::mapMariaDbTypeToJdbc(const SQLString& str)
  {
    SQLString type(str);

    type.toUpperCase();

    if (type.compare("BIT") == 0) {
      return Types::BIT;
    }
    else if (type.compare("TINYINT") == 0) {
      return Types::TINYINT;
    }
    else if (type.compare("SMALLINT") == 0) {
      return Types::SMALLINT;
    }
    else if (type.compare("MEDIUMINT") == 0) {
      return Types::INTEGER;
    }
    else if (type.compare("INT") == 0) {
      return Types::INTEGER;
    }
    else if (type.compare("INTEGER") == 0) {
      return Types::INTEGER;
    }
    else if (type.compare("LONG") == 0) {
      return Types::INTEGER;
    }
    else if (type.compare("BIGINT") == 0) {
      return Types::BIGINT;
    }
    else if (type.compare("INT24") == 0) {
      return Types::INTEGER;
    }
    else if (type.compare("REAL") == 0) {
      return Types::DOUBLE;
    }
    else if (type.compare("FLOAT") == 0) {
      return Types::FLOAT;
    }
    else if (type.compare("DECIMAL") == 0) {
      return Types::DECIMAL;
    }
    else if (type.compare("NUMERIC") == 0) {
      return Types::NUMERIC;
    }
    else if (type.compare("DOUBLE") == 0) {
      return Types::DOUBLE;
    }
    else if (type.compare("CHAR") == 0) {
      return Types::CHAR;
    }
    else if (type.compare("VARCHAR") == 0) {
      return Types::VARCHAR;
    }
    else if (type.compare("DATE") == 0) {
      return Types::DATE;
    }
    else if (type.compare("TIME") == 0) {
      return Types::TIME;
    }
    else if (type.compare("YEAR") == 0) {
      return Types::SMALLINT;
    }
    else if (type.compare("TIMESTAMP") == 0) {
      return Types::TIMESTAMP;
    }
    else if (type.compare("DATETIME") == 0) {
      return Types::TIMESTAMP;
    }
    else if (type.compare("TINYBLOB") == 0) {
      return Types::BINARY;
    }
    else if (type.compare("BLOB") == 0) {
      return Types::LONGVARBINARY;
    }
    else if (type.compare("MEDIUMBLOB") == 0) {
      return Types::LONGVARBINARY;
    }
    else if (type.compare("LONGBLOB") == 0) {
      return Types::LONGVARBINARY;
    }
    else if (type.compare("TINYTEXT") == 0) {
      return Types::VARCHAR;
    }
    else if (type.compare("TEXT") == 0) {
      return Types::LONGVARCHAR;
    }
    else if (type.compare("MEDIUMTEXT") == 0) {
      return Types::LONGVARCHAR;
    }
    else if (type.compare("LONGTEXT") == 0) {
      return Types::LONGVARCHAR;
    }
    else if (type.compare("ENUM") == 0) {
      return Types::VARCHAR;
    }
    else if (type.compare("SET") == 0) {
      return Types::VARCHAR;
    }
    else if (type.compare("GEOMETRY") == 0) {
      return Types::LONGVARBINARY;
    }
    else if (type.compare("VARBINARY") == 0) {
      return Types::VARBINARY;
    }
    else {
      return Types::OTHER;
    }
  }

  std::tuple<SQLString, SQLString> CallableParameterMetaData::queryMetaInfos(bool isFunction)
  {
    SQLString paramList;
    SQLString functionReturn;

    try {
      SQLString sql("select param_list, returns, db, type from mysql.proc where name=? and db=");
      sql.append(!database.empty() ? "?" : "DATABASE()");
      std::unique_ptr<PreparedStatement> preparedStatement(con->prepareStatement(sql));

      preparedStatement->setString(1, name);
      if (!database.empty()) {
        preparedStatement->setString(2, database);
      }

      std::unique_ptr<ResultSet> rs(preparedStatement->executeQuery()); {
        if (!rs->next()) {
          throw SQLException(
            (isFunction ? "function `" : "procedure `") + name + "` does not exist");
        }
        paramList= rs->getString(1);
        functionReturn= rs->getString(2);
        database= rs->getString(3);

        this->isFunction= (rs->getString(4).compare("FUNCTION") == 0);
        return std::make_tuple(paramList, functionReturn);
      }

    }
    catch (SQLException &/*sqlSyntaxErrorException*/) {
      throw SQLException("Access to metaData informations not granted for current user. Consider grant select access to mysql.proc "
        " or avoid using parameter by name");// , sqlSyntaxErrorException);
    }
  }

  void CallableParameterMetaData::parseFunctionReturnParam(const SQLString& functionReturn)
  {
    if (functionReturn.empty()) {
      throw SQLException(name +"is not a function returning value");
    }

    std::smatch matcher;
    if (!std::regex_search(StringImp::get(functionReturn), matcher, RETURN_PATTERN)) {
      throw SQLException("can not parse return value definition :"+functionReturn);
    }
    CallParameter& callParameter= params[0];
    callParameter.setOutput(true);
    callParameter.setSigned(matcher[1].str().empty());
    SQLString typeName(matcher[2].str());
    callParameter.setTypeName(typeName.trim());
    callParameter.setSqlType(mapMariaDbTypeToJdbc(callParameter.getTypeName()));
    SQLString scale(matcher[3].str());
    if (!scale.empty()) {
      scale= replace(scale, "(", "");
      scale= replace(scale, ")", "");
      scale= replace(scale, " ", "");
      callParameter.setScale(std::stoi(StringImp::get(scale)));
    }
  }

  void CallableParameterMetaData::parseParamList(bool isFunction, const SQLString& paramList)
  {
    params.clear();
    if (isFunction) {
      // output parameter
      params.push_back(CallParameter());
    }

    std::smatch matcher2;
    std::string pList(StringImp::get(paramList));

    while (std::regex_search(pList, matcher2, PARAMETER_PATTERN))
    {
      CallParameter callParameter;

      SQLString match(matcher2[1].str());

      match.trim().toUpperCase();

      /* Direction */
      if (match.empty() || match.compare("IN") == 0) {
        callParameter.setInput(true);
      }
      else if (match.compare("OUT") == 0) {
        callParameter.setOutput(true);
      }
      else if (match.compare("INOUT") == 0) {
        callParameter.setInput(true);
        callParameter.setOutput(true);
      }
      else {
        throw SQLException("unknown parameter direction "+ match +"for "+ matcher2[2].str());
      }

      match= matcher2[2].str();
      callParameter.setName(match.trim());
      callParameter.setSigned(matcher2[3].str().empty());
      match= matcher2[4].str();
      callParameter.setTypeName(match.trim().toUpperCase());
      callParameter.setSqlType(mapMariaDbTypeToJdbc(callParameter.getTypeName()));

      SQLString scale= matcher2[5].str();
      scale.trim();

      if (!scale.empty()) {
        scale= replace(scale, "(", "");
        scale= replace(scale, ")", "");
        scale= replace(scale, " ", "");
        if ((scale.find_first_of(",") != std::string::npos)) {
          scale= scale.substr(0, scale.find_first_of(","));
        }
        callParameter.setScale(std::stoi(StringImp::get(scale)));
      }
      params.push_back(callParameter);
      pList= matcher2.suffix();
    }
  }

  /**
    * Read procedure metadata from mysql.proc table(column param_list).
    *
    * @throws SQLException if data doesn't correspond.
    */
  void CallableParameterMetaData::readMetadata()
  {
    if (valid) {
      return;
    }

    std::tuple<SQLString, SQLString> metaInfos= queryMetaInfos(isFunction);
    SQLString paramList= std::get<0>(metaInfos);
    SQLString functionReturn= std::get<1>(metaInfos);

    parseParamList(isFunction, paramList);


    if (isFunction) {
      parseFunctionReturnParam(functionReturn);
    }
  }

  uint32_t CallableParameterMetaData::getParameterCount()
  {
    return static_cast<uint32_t>(params.size());
  }

  CallParameter& CallableParameterMetaData::getParam(uint32_t index)
  {
    if (index < 1 || index > params.size()) {
      throw SQLException("invalid parameter index "+index);
    }
    readMetadataFromDbIfRequired();
    return params[index - 1];
  }

  int32_t CallableParameterMetaData::isNullable(uint32_t param)
  {
    return getParam(param).getCanBeNull();
  }

  bool CallableParameterMetaData::isSigned(uint32_t param)
  {
    return getParam(param).isSigned();
  }

  int32_t CallableParameterMetaData::getPrecision(uint32_t param)
  {
    return getParam(param).getPrecision();
  }

  int32_t CallableParameterMetaData::getScale(uint32_t param)
  {
    return getParam(param).getScale();
  }

  int32_t CallableParameterMetaData::getParameterType(uint32_t param)
  {
    return getParam(param).getSqlType();
  }

  SQLString CallableParameterMetaData::getParameterTypeName(uint32_t param)
  {
    return getParam(param).getTypeName();
  }

  SQLString CallableParameterMetaData::getParameterClassName(uint32_t param)
  {
    return getParam(param).getClassName();
  }

  /**
    * Get mode info.
    *
    * <ul>
    *   <li>0 : unknown
    *   <li>1 : IN
    *   <li>2 : INOUT
    *   <li>4 : OUT
    * </ul>
    *
    * @param param parameter index
    * @return mode information
    * @throws SQLException if index is wrong
    */
  int32_t CallableParameterMetaData::getParameterMode(uint32_t param)
  {
    CallParameter callParameter= getParam(param);
    if (callParameter.isInput()&&callParameter.isOutput()) {
      return parameterModeInOut;
    }
    if (callParameter.isInput()) {
      return parameterModeIn;
    }
    if (callParameter.isOutput()) {
      return parameterModeOut;
    }
    return parameterModeUnknown;
  }

  const SQLString& CallableParameterMetaData::getName(uint32_t param)
  {
    return getParam(param).getName();
  }
}
}
