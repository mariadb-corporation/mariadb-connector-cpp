// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (c) 2025 MariaDB Corporation plc


#include "SQLString.h"
#include "pimpls.h"
#include <vector>

namespace mariadb
{
  class unique_key
  {
    SQLString schema;
    SQLString table;
    std::vector<std::size_t> keyField;
    int32_t keyType= -1;
    uint32_t cursorColumnCount= 0;
    ResultSetMetaData* cursorMeta= nullptr;
    std::size_t keyFieldNamesLen= 0; // total len of all key field names
    SQLString where;

    void chooseIndex(Protocol* protocol, uint32_t priCount, uint32_t uniqueCount);
    void cacheKeyFields(uint32_t cursorPriCount);

  public:
    unique_key(Protocol* protocol, ResultSetMetaData* meta);
    // Only reads cursor's schema and catalog and verifies that only 1 table is used
    unique_key(ResultSetMetaData* meta);

    inline const SQLString& getSchema() const { return schema; }
    inline const SQLString& getTable() const { return table; }
    // Returns true if we have index and not simply using all table fields to identify the record
    inline bool             isIndex() const { return keyType > 0; }
    // We remeber if index can't be find for this cursor. Just in case some automated system may bang its head
    // against the wall - we won't re-send queries
    inline bool             exists() const { return keyType >= 0; }
    inline uint32_t         fieldCount() const { return (keyType > 0 ? static_cast<uint32_t>(keyField.size())
                                                                    : cursorColumnCount); }
    const char* fieldName(std::size_t i) const;
    inline uint32_t fieldIndex(std::size_t i) const {
      return static_cast<uint32_t>(keyType == 0 ? i : keyField[i]);
    }
    // Generating where with placeholders
    SQLString   getWhereClause() const;
    std::size_t getWhereClauseLenEstim(std::size_t fieldLenEstim) const;
    //inline bool 
  };
}

