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


#include "ExceptionFactory.h"
#include "SqlStates.h"
#include "MariaDBWarning.h"

namespace sql
{
namespace mariadb
{

  const std::array<int32_t, 3>LOCK_DEADLOCK_ERROR_CODE={ 1205,1213,1614 };
  /**
    * Helper to throw exception.
    *
    * @param exception exception
    * @param connection current connection
    * @param statement current statement
    * @throws SQLException exception
    */
  void ExceptionMapper::throwException(SQLException &exception, MariaDbConnection* connection, MariaDbStatement* statement) {
    throw getException(exception, connection, statement, false);
  }

  SQLException ExceptionMapper::connException(const SQLString& message) {
    return connException(message, NULL);
  }

  SQLException ExceptionMapper::connException(const SQLString& message, std::exception* cause) {
    return get(message, CONNECTION_EXCEPTION.getSqlState(), -1, cause, false);
  }

  /**
    * Helper to decorate exception with associate subclass of {@link SQLException} exception.
    *
    * @param exception exception
    * @param connection current connection
    * @param statement current statement
    * @param timeout was timeout on query
    * @return SQLException exception
    */
  SQLException ExceptionMapper::getException(SQLException &exception, MariaDbConnection* connection, MariaDbStatement* statement, bool timeout)
  {
    SQLString message("(conn=");
    SQLString errMsg(exception.getMessage());

    if ((errMsg.find_first_of("\n") != std::string::npos))
    {
      errMsg= errMsg.substr(0, errMsg.find_first_of("\n"));
    }
    if (connection != NULL)
    {
      message.append(std::to_string(connection->getServerThreadId())).append(") ").append(errMsg);
    }
    else if (statement != NULL) {
      message.append(std::to_string(statement->getServerThreadId())).append(") ")
        .append(errMsg);
    }
    else {
      message= errMsg;
    }

    SQLException sqlException;

    if (!exception.getSQLState().empty())
    {
      if (connection != NULL && (errMsg.find_first_of("\n") != std::string::npos))
      {
        if (connection->includeDeadLockInfo())
        {
          try
          {
            Unique::Statement stmt(connection->createStatement());
            Unique::ResultSet rs(stmt->executeQuery("SHOW ENGINE INNODB STATUS"));

            if (rs->next())
            {
              message.append("\nDeadlock information: ").append(rs->getString(3));
            }
          }
          catch (SQLException& /*sqle*/) {
            // eat
          }
        }

        if (connection->includeThreadsTraces())
        {
#ifdef WE_FOUND_WHAT_TO_USE_HERE
          message.append("\n\ncurrent threads: ");
          Thread.getAllStackTraces()
            .forEach(
            (thread, traces)->{
            message
              .append("\n  name:\"")
              .append(thread.getName())
              .append("\" pid:")
              .append(thread.getId())
              .append(" status:")
              .append(thread.getState());
            for (int32_t i= 0; i <traces.length; i++) {
              message.append("\n    ").append(traces[i]);
            }
          });
#endif
        }
      }
      sqlException= get(message, exception.getSQLState(), exception.getErrorCode(), &exception,
          timeout);

      SQLException* nextException= exception.getNextException();
      if (nextException != NULL) {
        sqlException.setNextException(*nextException);
      }
    }
    else {
      sqlException= exception;
    }

    SQLString sqlState= exception.getSQLState();
    SqlStates state= SqlStates::fromString(sqlState);

    if (connection != NULL)
    {
      if (CONNECTION_EXCEPTION.getSqlState().compare(state.getSqlState()) == 0)
      {
        connection->setHostFailed();
        if (connection->pooledConnection)
        {
          connection->pooledConnection->fireConnectionErrorOccured(sqlException);
        }
      }
      else if (connection->pooledConnection && statement != NULL) {
        connection->pooledConnection->fireStatementErrorOccured(statement, sqlException);
      }
    }

    return sqlException;
  }

  /**
    * Check connection exception to report to poolConnection listeners.
    *
    * @param exception current exception
    * @param connection current connection
    */
  void ExceptionMapper::checkConnectionException(SQLException& exception, MariaDbConnection* connection)
  {
    if (exception.getSQLState().empty() != true)
    {
      SqlStates state(SqlStates::fromString(exception.getSQLState()));

      if (CONNECTION_EXCEPTION.getSqlState().compare(state.getSqlState()) == 0)
      {
        connection->setHostFailed();
        if (connection->pooledConnection) {
          connection->pooledConnection->fireConnectionErrorOccured(exception);
        }


      }
    }
  }

  /**
    * Helper to decorate exception with associate subclass of {@link SQLException} exception.
    *
    * @param message exception message
    * @param sqlState sqlstate
    * @param errorCode errorCode
    * @param exception cause
    * @param timeout was timeout on query
    * @return SQLException exception
    */
  SQLException ExceptionMapper::get(const SQLString& message, const SQLString& sqlState, int32_t errorCode,
                                    const std::exception *exception, bool timeout)
  {
    const SqlStates state(SqlStates::fromString(sqlState));

    if (state.getSqlState().compare(DATA_EXCEPTION.getSqlState()) == 0) {
      return SQLDataException(message, sqlState, errorCode, exception);
    }
    else if(state.getSqlState().compare(FEATURE_NOT_SUPPORTED.getSqlState()) == 0) {
      return SQLFeatureNotSupportedException(message, sqlState, errorCode, exception);
    }
    else if(state.getSqlState().compare(CONSTRAINT_VIOLATION.getSqlState()) == 0) {
      return SQLIntegrityConstraintViolationException(
        message, sqlState, errorCode, exception);
    }
    else if(state.getSqlState().compare(INVALID_AUTHORIZATION.getSqlState()) == 0) {
      return SQLInvalidAuthorizationSpecException(message, sqlState, errorCode, exception);
    }
    else if(state.getSqlState().compare(CONNECTION_EXCEPTION.getSqlState()) == 0) {
      return SQLNonTransientConnectionException(message, sqlState, errorCode, exception);
    }
    else if(state.getSqlState().compare(SYNTAX_ERROR_ACCESS_RULE.getSqlState()) == 0) {
      return SQLSyntaxErrorException(message, sqlState, errorCode, exception);
    }
    else if(state.getSqlState().compare(TRANSACTION_ROLLBACK.getSqlState()) == 0) {
      return SQLTransactionRollbackException(message, sqlState, errorCode, exception);
    }
    else if(state.getSqlState().compare(WARNING.getSqlState()) == 0) {
      /* TODO: need to think about that */
      //return MariaDBWarning(message, sqlState, errorCode, exception);
      return SQLException(message, sqlState, errorCode, exception);
    }
    else if(state.getSqlState().compare(INTERRUPTED_EXCEPTION.getSqlState()) == 0) {
      if(timeout && (sqlState.compare("70100") == 0))
      {
        return SQLTimeoutException(message, sqlState, errorCode, exception);
      }
      else if(INSTANCEOF(exception, const SQLNonTransientConnectionException*))
      {
        return SQLNonTransientConnectionException(message, sqlState, errorCode, exception);
      }
      return SQLTransientException(message, sqlState, errorCode, exception);
    }
    else if(state.getSqlState().compare(TIMEOUT_EXCEPTION.getSqlState()) == 0) {
      return SQLTimeoutException(message, sqlState, errorCode, exception);
    }
    else if (state.getSqlState().compare(UNDEFINED_SQLSTATE.getSqlState()) == 0) {
      if (INSTANCEOF(exception, const SQLNonTransientConnectionException*))
      {
        return SQLNonTransientConnectionException(message, sqlState, errorCode, exception);
      }
      return SQLException(message, sqlState, errorCode, exception);
    }
    else {
      return SQLException(message, sqlState, errorCode, exception);
    }
  }

  SQLException ExceptionMapper::getSqlException(const SQLString& message, std::runtime_error& exception)
  {
    return SQLException(message, "", 0, &exception);
  }

  SQLException ExceptionMapper::getSqlException(const SQLString& message, const SQLString& sqlState, std::runtime_error& exception) {
    return SQLException(message, sqlState, 0, &exception);
  }

  SQLException ExceptionMapper::getSqlException(const SQLString& message) {
    return SQLException(message);
  }

  SQLFeatureNotSupportedException ExceptionMapper::getFeatureNotSupportedException(const SQLString& message) {
    return SQLFeatureNotSupportedException(message);
  }

  /**
    * Mapp code to State.
    *
    * @param code code
    * @return String
    */
  SQLString ExceptionMapper::mapCodeToSqlState(int32_t code) {
    switch (code) {
    case 1022:
      return "23000";
    case 1037:
      return "HY001";
    case 1038:
      return "HY001";
    case 1040:
      return "08004";
    case 1042:
      return "08S01";
    case 1043:
      return "08S01";
    case 1044:
      return "42000";
    case 1045:
      return "28000";
    case 1047:
      return "HY000";
    case 1050:
      return "42S01";
    case 1051:
      return "42S02";
    case 1052:
      return "23000";
    case 1053:
      return "08S01";
    case 1054:
      return "42S22";
    case 1055:
      return "42000";
    case 1056:
      return "42000";
    case 1057:
      return "42000";
    case 1058:
      return "21S01";
    case 1059:
      return "42000";
    case 1060:
      return "42S21";
    case 1061:
      return "42000";
    case 1062:
      return "23000";
    case 1063:
      return "42000";
    case 1064:
      return "42000";
    case 1065:
      return "42000";
    case 1066:
      return "42000";
    case 1067:
      return "42000";
    case 1068:
      return "42000";
    case 1069:
      return "42000";
    case 1070:
      return "42000";
    case 1071:
      return "42000";
    case 1072:
      return "42000";
    case 1073:
      return "42000";
    case 1074:
      return "42000";
    case 1075:
      return "42000";
    case 1080:
      return "08S01";
    case 1081:
      return "08S01";
    case 1082:
      return "42S12";
    case 1083:
      return "42000";
    case 1084:
      return "42000";
    case 1090:
      return "42000";
    case 1091:
      return "42000";
    case 1101:
      return "42000";
    case 1102:
      return "42000";
    case 1103:
      return "42000";
    case 1104:
      return "42000";
    case 1106:
      return "42000";
    case 1107:
      return "42000";
    case 1109:
      return "42S02";
    case 1110:
      return "42000";
    case 1112:
      return "42000";
    case 1113:
      return "42000";
    case 1115:
      return "42000";
    case 1118:
      return "42000";
    case 1120:
      return "42000";
    case 1121:
      return "42000";
    case 1129:
      return "HY000";
    case 1130:
      return "HY000";
    case 1131:
      return "42000";
    case 1132:
      return "42000";
    case 1133:
      return "42000";
    case 1136:
      return "21S01";
    case 1138:
      return "42000";
    case 1139:
      return "42000";
    case 1140:
      return "42000";
    case 1141:
      return "42000";
    case 1142:
      return "42000";
    case 1143:
      return "42000";
    case 1144:
      return "42000";
    case 1145:
      return "42000";
    case 1146:
      return "42S02";
    case 1147:
      return "42000";
    case 1148:
      return "42000";
    case 1149:
      return "42000";
    case 1152:
      return "08S01";
    case 1153:
      return "08S01";
    case 1154:
      return "08S01";
    case 1155:
      return "08S01";
    case 1156:
      return "08S01";
    case 1157:
      return "08S01";
    case 1158:
      return "08S01";
    case 1159:
      return "08S01";
    case 1160:
      return "08S01";
    case 1161:
      return "08S01";
    case 1162:
      return "42000";
    case 1163:
      return "42000";
    case 1164:
      return "42000";
    case 1166:
      return "42000";
    case 1167:
      return "42000";
    case 1169:
      return "23000";
    case 1170:
      return "42000";
    case 1171:
      return "42000";
    case 1172:
      return "42000";
    case 1173:
      return "42000";
    case 1177:
      return "42000";
    case 1178:
      return "42000";
    case 1179:
      return "25000";
    case 1184:
      return "08S01";
    case 1189:
      return "08S01";
    case 1190:
      return "08S01";
    case 1203:
      return "42000";
    case 1205:
      return "41000";
    case 1207:
      return "25000";
    case 1211:
      return "42000";
    case 1213:
      return "40001";
    case 1216:
      return "23000";
    case 1217:
      return "23000";
    case 1218:
      return "08S01";
    case 1222:
      return "21000";
    case 1226:
      return "42000";
    case 1230:
      return "42000";
    case 1231:
      return "42000";
    case 1232:
      return "42000";
    case 1234:
      return "42000";
    case 1235:
      return "42000";
    case 1239:
      return "42000";
    case 1241:
      return "21000";
    case 1242:
      return "21000";
    case 1247:
      return "42S22";
    case 1248:
      return "42000";
    case 1249:
      return "01000";
    case 1250:
      return "42000";
    case 1251:
      return "08004";
    case 1252:
      return "42000";
    case 1253:
      return "42000";
    case 1261:
      return "01000";
    case 1262:
      return "01000";
    case 1263:
      return "01000";
    case 1264:
      return "01000";
    case 1265:
      return "01000";
    case 1280:
      return "42000";
    case 1281:
      return "42000";
    case 1286:
      return "42000";
    default:
      return NULL;
    }
  }
}
}
