// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (c) 2025 MariaDB Corporation plc


#include "unique_key.h"
#include "Protocol.h"
#include "ResultSetMetaData.h"

#define EXPECTED_MAX_INDEX_COLUMNS_COUNT 4ULL
#define AVG_FIELD_VALUE_ESTIMATION 10

namespace mariadb
{
  /* TODO: if there are >1 of unique keys, this will go wrong */
  void unique_key::chooseIndex(Protocol* protocol, uint32_t cursorPriCount, uint32_t cursorUniqueCount)
  {
    uint32_t count= 0, totalPriCount= 0, totalUniqueCount= 0;
    SQLString query;
    MYSQL_RES *res;
    MYSQL_FIELD *field;

    query.reserve(sizeof("SELECT * FROM `") + schema.length() + 3/*`.`*/ + table.length() + sizeof("` LIMIT 0"));
    query.assign("SELECT * FROM `").append(schema).append("`.`").append(table).append("` LIMIT 0");

    std::lock_guard<std::mutex> localScopeLock(protocol->getLock());
    protocol->safeRealQuery(query);
    if ((res= mysql_store_result(protocol->getCHandle())) != nullptr) {
      count= mysql_num_fields(res);
      for (uint32_t i= 0; i < count; ++i) {
        field= mysql_fetch_field_direct(res, i);
        if (field->flags & PRI_KEY_FLAG) {
          ++totalPriCount;
        }
        if (field->flags & UNIQUE_KEY_FLAG) {
          ++totalUniqueCount;
        }
      }
      mysql_free_result(res);
    }

    if (cursorPriCount && cursorPriCount == totalPriCount) {
      keyType= PRI_KEY_FLAG;
      cacheKeyFields(cursorPriCount);
    }
    /* We use unique index if we do not have primary. TODO: If there are more than one unique index - we are in trouble */
    else if (cursorUniqueCount && cursorUniqueCount >= totalUniqueCount) {
      keyType= UNIQUE_KEY_FLAG;
      cacheKeyFields(cursorUniqueCount);
    }
    /* if no primary or unique key is in the cursor, the cursor must contain all
       columns from the table */
    else if (count == cursorColumnCount)
    {
      keyType= 0;
    }
  }


  void unique_key::cacheKeyFields(uint32_t keyColumnsCount)
  {
    keyField.reserve(keyColumnsCount);
    keyFieldNamesLen= 0;
    for (uint32_t i= 0; i < cursorColumnCount; ++i) {
      const MYSQL_FIELD* field= cursorMeta->getField(i);
      if (field->flags & keyType) {
        keyFieldNamesLen+= field->org_name_length;
        keyField.push_back(i);
      }
    }
  }

  // Only reads cursor's schema and catalog and verifies that only 1 table is used. No index info
  unique_key::unique_key(ResultSetMetaData* meta)
    : cursorColumnCount(meta->getColumnCount())
  {
    char* tableName= nullptr, * schemaName= nullptr;
    for (uint32_t i= 0; i < cursorColumnCount; ++i) {
      const MYSQL_FIELD* field= meta->getField(i);
      if (field->db) {
        if (!schemaName) {
          schemaName= field->db;
        }
        else if (strcmp(schemaName, field->db)) {
          throw SQLException("Couldn't identify unique schema name", "HY000");
        }
      }
      if (field->org_table) {
        if (!tableName) {
          tableName= field->org_table;
        }
        else if (strcmp(tableName, field->org_table)) {
          throw SQLException("Couldn't identify unique table name", "HY000");
        }
      }
    }
    schema.assign(schemaName);
    table.assign(tableName);
  }

  unique_key::unique_key(Protocol* protocol, ResultSetMetaData* meta)
    : cursorColumnCount(meta->getColumnCount())
    , cursorMeta(meta)
  {
    uint32_t primaryCount= 0, uniqueCount= 0;
    char *tableName= nullptr, *schemaName= nullptr;

    for (uint32_t i= 0; i < cursorColumnCount; ++i) {
      const MYSQL_FIELD* field= meta->getField(i);
      if (field->db) {
        if (!schemaName) {
          schemaName= field->db;
        }
        else if (strcmp(schemaName, field->db)) {
          throw SQLException("Couldn't identify unique schema name", "HY000");
        }
      }
      if (field->org_table) {
        if (!tableName) {
          tableName= field->org_table;
        }
        else if (strcmp(tableName, field->org_table)) {
          throw SQLException("Couldn't identify unique table name", "HY000");
        }
      }
      if (field->flags & PRI_KEY_FLAG) {
        ++primaryCount;
      }
      if (field->flags & UNIQUE_KEY_FLAG) {
        ++uniqueCount;
      }
      keyFieldNamesLen+= field->org_name_length;
    }
    schema.assign(schemaName);
    table.assign(tableName);

    chooseIndex(protocol, primaryCount, uniqueCount);
  }


  const char* unique_key::fieldName(std::size_t i) const
  {
    return cursorMeta->getField(keyType == 0 ? i : keyField[i])->org_name;
  }


  SQLString unique_key::getWhereClause() const
  {
    SQLString where;
    auto count= fieldCount();

    // Making estimation with value len=1 for parameter marker '?'
    where.reserve(getWhereClauseLenEstim(1));
    where.assign("WHERE ", 6);

    for (uint32_t i= 0; i < count; ++i) {
      auto field= cursorMeta->getField(keyType == 0 ? i : keyField[i]);
      if (i) {
        where.append(" AND ", 5);
      }
      where.append(field->name, field->name_length).append("=?", 2);
    }
    return where;
  }


  std::size_t unique_key::getWhereClauseLenEstim(std::size_t fieldLenEstim= AVG_FIELD_VALUE_ESTIMATION) const
  {
    auto count= fieldCount();
    return 6/*WHERE */ + keyFieldNamesLen + (count*(AVG_FIELD_VALUE_ESTIMATION + 1) + (count - 1) * 5)/*=_VALUE_ AND */;
  }
}

