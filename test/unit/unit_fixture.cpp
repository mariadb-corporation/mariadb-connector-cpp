/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
 *               2020, 2022 MariaDB Corporation AB
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0, as
 * published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an
 * additional permission to link the program and your derivative works
 * with the separately licensed software that they have included with
 * MySQL.
 *
 * Without limiting anything contained in the foregoing, this file,
 * which is part of MySQL Connector/C++, is also subject to the
 * Universal FOSS Exception, version 1.0, a copy of which can be found at
 * http://oss.oracle.com/licenses/universal-foss-exception.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
 */



#include "unit_fixture.h"
#include <cstdio>

#ifndef L64
#ifdef _WIN32
#define L64(x) x##i64
#else
#define L64(x) x##LL
#endif
#endif

namespace testsuite
{

Driver * unit_fixture::driver= nullptr;

std::vector< columndefinition > unit_fixture::columns = {
  {"BIT", "BIT", sql::DataType::BIT, "0", false, 1, 0, true, "NULL", 0, "NO", false},

  {"BIT", "BIT NOT NULL", sql::DataType::BIT, "1", false, 1, 0, false, "", 0, "NO", false},
  {"BIT", "BIT(5) NOT NULL", sql::DataType::BIT, "0", false, 5, 0, false, "", 0, "NO", false},
  {"BIT", "BIT(8)", sql::DataType::BIT, "0", false, 8, 0, true, "NULL", 0, "NO", false},
  {"TINYINT", "TINYINT", sql::DataType::TINYINT, "127", true, 3, 0, true, "NULL", 0, "NO", false, "127"},
  {"TINYINT", "TINYINT NOT NULL", sql::DataType::TINYINT, "127", true, 3, 0, false, "", 0, "NO", false, "127"},
  {"TINYINT", "TINYINT NOT NULL DEFAULT -1", sql::DataType::TINYINT, "12", true, 3, 0, false, "-1", 0, "NO", false, "12"},
  {"BIT", "TINYINT(1)", sql::DataType::BIT, "3", true, 3, 0, true, "NULL", 0, "NO", false, "3"},
  {"TINYINT", "TINYINT UNSIGNED", sql::DataType::TINYINT, "255", false, 3, 0, true, "NULL", 0, "NO", false, "255"},
  {"TINYINT", "TINYINT ZEROFILL", sql::DataType::TINYINT, "1", false, 3, 0, true, "NULL", 0, "NO", false},
  // Alias of BOOLEAN
  {"TINYINT", "BOOLEAN", sql::DataType::BIT, "1", true, 3, 0, true, "NULL", 0, "NO", false, "1"},

  {"TINYINT", "BOOLEAN NOT NULL", sql::DataType::BIT, "2", true, 3, 0, false, "", 0, "NO", false, "2"},
  {"TINYINT", "BOOLEAN DEFAULT 0", sql::DataType::BIT, "3", true, 3, 0, true, "0", 0, "NO", false, "3"},
  {"SMALLINT", "SMALLINT", sql::DataType::SMALLINT, "-32768", true, 5, 0, true, "NULL", 0, "NO", true, "-32768"},
  {"SMALLINT", "SMALLINT NOT NULL", sql::DataType::SMALLINT, "32767", true, 5, 0, false, "", 0, "NO", false, "32767"},
  {"SMALLINT", "SMALLINT NOT NULL DEFAULT -1", sql::DataType::SMALLINT, "-32768", true, 5, 0, false, "-1", 0, "NO", true},
  {"SMALLINT", "SMALLINT(3)", sql::DataType::SMALLINT, "-32768", true, 5, 0, true, "NULL", 0, "NO", true},
  {"SMALLINT", "SMALLINT UNSIGNED", sql::DataType::SMALLINT, "65535", false, 5, 0, true, "NULL", 0, "NO", false, "65535"},
  {"SMALLINT", "SMALLINT ZEROFILL", sql::DataType::SMALLINT, "123", false, 5, 0, true, "NULL", 0, "NO", false},
  {"SMALLINT", "SMALLINT UNSIGNED ZEROFILL DEFAULT 101", sql::DataType::SMALLINT, "123", false, 5, 0, true, "00101", 0, "NO", false},
  {"MEDIUMINT", "MEDIUMINT", sql::DataType::INTEGER, "-8388608", true, 7, 0, true, "NULL", 0, "NO", true, "-8388608"},
  {"MEDIUMINT", "MEDIUMINT NOT NULL", sql::DataType::INTEGER, "-8388608", true, 7, 0, false, "", 0, "NO", true},
  {"MEDIUMINT", "MEDIUMINT(1)", sql::DataType::INTEGER, "2", true, 7, 0, true, "NULL", 0, "NO", false},
  {"MEDIUMINT", "MEDIUMINT(2) DEFAULT 12", sql::DataType::INTEGER, "2", true, 7, 0, true, "12", 0, "NO", false},
  {"MEDIUMINT", "MEDIUMINT UNSIGNED", sql::DataType::INTEGER, "16777215", false, 8, 0, true, "NULL", 0, "NO", false, "16777215"},
  {"MEDIUMINT", "MEDIUMINT UNSIGNED ZEROFILL", sql::DataType::INTEGER, "1677721", false, 8, 0, true, "NULL", 0, "NO", false},
  // Alias of INTEGER
  {"INT", "INTEGER", sql::DataType::INTEGER, "2147483647", true, 10, 0, true, "NULL", 0, "NO", false, "2147483647"},
  {"INT", "INTEGER NOT NULL", sql::DataType::INTEGER, "2147483647", true, 10, 0, false, "", 0, "NO", false},
  {"INT", "INTEGER(1)", sql::DataType::INTEGER, "3", true, 10, 0, true, "NULL", 0, "NO", false},
  {"INT", "INT UNSIGNED", sql::DataType::INTEGER, "4294967295", false, 10, 0, true, "NULL", 0, "NO", false, "4294967295"},
  // If you specify ZEROFILL for a numeric column, MySQL automatically adds the UNSIGNED  attribute to the column.
  {"INT", "INT(4) UNSIGNED ZEROFILL", sql::DataType::INTEGER, "1", false, 10, 0, true, "NULL", 0, "NO", false},
  {"INT", "INT(4) SIGNED DEFAULT -123", sql::DataType::INTEGER, "1", true, 10, 0, true, "-123", 0, "NO", false},
  {"BIGINT", "BIGINT", sql::DataType::BIGINT, "-9223372036854775808", true, 19, 0, true, "NULL", 0, "NO", true, "-9223372036854775808"},

  {"BIGINT", "BIGINT NOT NULL", sql::DataType::BIGINT, "-9223372036854775808", true, 19, 0, false, "", 0, "NO", true},
  {"BIGINT", "BIGINT UNSIGNED", sql::DataType::BIGINT, "18446744073709551615", false, 20, 0, true, "NULL", 0, "NO", false, "18446744073709551615"},
  {"BIGINT", "BIGINT UNSIGNED", sql::DataType::BIGINT, "18446744073709551615", false, 20, 0, true, "NULL", 0, "NO", false},
  {"BIGINT", "BIGINT(4) ZEROFILL DEFAULT 10101", sql::DataType::BIGINT, "2", false, 20, 0, true, "10101", 0, "NO", false},
  {"FLOAT", "FLOAT", sql::DataType::REAL, "-1.01", true, 12, 0, true, "NULL", 0, "NO", true},
  {"FLOAT", "FLOAT NOT NULL", sql::DataType::REAL, "-1.01", true, 12, 0, false, "", 0, "NO", true},
  {"FLOAT", "FLOAT UNSIGNED", sql::DataType::REAL, "1.01", false, 12, 0, true, "NULL", 0, "NO", false},
  {"FLOAT", "FLOAT(5,3) UNSIGNED ZEROFILL", sql::DataType::REAL, "1.01", false, 5, 3, true, "NULL", 0, "NO", false},
  {"FLOAT", "FLOAT(6)", sql::DataType::REAL, "1.01", true, 12, 0, true, "NULL", 0, "NO", false},
  {"FLOAT", "FLOAT(9) DEFAULT 1.01", sql::DataType::REAL, "1.01", true, 12, 0, true, "1.01", 0, "NO", false},
  {"DOUBLE", "DOUBLE", sql::DataType::DOUBLE, "-1.01", true, 22, 0, true, "NULL", 0, "NO", true},
  {"DOUBLE", "DOUBLE NOT NULL", sql::DataType::DOUBLE, "-1.01", true, 22, 0, false, "", 0, "NO", true},
  {"DOUBLE", "DOUBLE UNSIGNED", sql::DataType::DOUBLE, "1.01", false, 22, 0, true, "NULL", 0, "NO", false},
  {"DOUBLE", "DOUBLE UNSIGNED DEFAULT 1.123", sql::DataType::DOUBLE, "1.01", false, 22, 0, true, "1.123", 0, "NO", false},
  {"DOUBLE", "DOUBLE(5,3) UNSIGNED ZEROFILL", sql::DataType::DOUBLE, "1.01", false, 5, 3, true, "NULL", 0, "NO", false},
  {"DECIMAL", "DECIMAL", sql::DataType::DECIMAL, "-1.01", true, 10, 0, true, "NULL", 0, "NO", true},
  {"DECIMAL", "DECIMAL NOT NULL", sql::DataType::DECIMAL, "-1.01", true, 10, 0, false, "", 0, "NO", true},
  {"DECIMAL", "DECIMAL UNSIGNED", sql::DataType::DECIMAL, "1.01", false, 10, 0, true, "NULL", 0, "NO", false},
  {"DECIMAL", "DECIMAL(5,3) UNSIGNED ZEROFILL", sql::DataType::DECIMAL, "1.01", false, 5, 3, true, "NULL", 0, "NO", false},
  {"DECIMAL", "DECIMAL(6,3) UNSIGNED ZEROFILL DEFAULT 34.56", sql::DataType::DECIMAL, "1.01", false, 6, 3, true, "034.560", 0, "NO", false},
  {"DATE", "DATE", sql::DataType::DATE, "2009-02-09", true, 10, 0, true, "NULL", 0, "NO", false, "2009-02-09"},
  {"DATE", "DATE NOT NULL", sql::DataType::DATE, "2009-02-12", true, 10, 0, false, "", 0, "NO", false},
  {"DATE", "DATE NOT NULL DEFAULT '2009-02-16'", sql::DataType::DATE, "2009-02-12", true, 10, 0, false, "'2009-02-16'", 0, "NO", false},
  {"DATETIME", "DATETIME", sql::DataType::TIMESTAMP, "2009-02-09 20:05:43", true, 19, 0, true, "NULL", 0, "NO", false, "2009-02-09 20:05:43"},
  {"DATETIME", "DATETIME NOT NULL", sql::DataType::TIMESTAMP, "2009-02-12 17:49:21", true, 19, 0, false, "", 0, "NO", false},
  {"DATETIME", "DATETIME NOT NULL DEFAULT '2009-02-12 21:36:54'", sql::DataType::TIMESTAMP, "2009-02-12 17:49:21", true, 19, 0, false, "'2009-02-12 21:36:54'", 0, "NO", false},
  // TODO this might be server dependent!

  {"TIMESTAMP", "TIMESTAMP", sql::DataType::TIMESTAMP, "2038-01-09 03:14:07", false, 19, 0, true, "'0000-00-00 00:00:00'", 0, "NO", false},
  {"TIME", "TIME", sql::DataType::TIME, "-838:59:59", true, 8, 0, true, "NULL", 0, "NO", true},
  {"TIME", "TIME NOT NULL", sql::DataType::TIME, "838:59:59", true, 8, 0, false, "", 0, "NO", false},
  {"TIME", "TIME DEFAULT '12:39:41'", sql::DataType::TIME, "-838:59:59", true, 8, 0, true, "'12:39:41'", 0, "NO", true},
  {"YEAR", "YEAR", sql::DataType::DATE/*Optional, DATE is default, SMALLINT otherwise*/, "1901", false, 4, 0, true, "NULL", 0, "NO", false},
  {"YEAR", "YEAR NOT NULL", sql::DataType::DATE, "1902", false, 4, 0, false, "", 0, "NO", false},
  {"YEAR", "YEAR(4)", sql::DataType::DATE, "2009", false, 4, 0, true, "NULL", 0, "NO", false, "2009"},

  {"YEAR", "YEAR(2)", sql::DataType::DATE, "1", false, 4, 0, true, "NULL", 0, "NO", false},
  {"YEAR", "YEAR(3) DEFAULT '2009'", sql::DataType::DATE, "1", false, 4, 0, true, "2009", 0, "NO", false},

  {"CHAR", "CHAR", sql::DataType::CHAR, "a", true, 1, 0, true, "NULL", 0, "NO", false, "a"},
  {"CHAR", "CHAR NOT NULL", sql::DataType::CHAR, "a", true, 1, 0, false, "", 0, "NO", false},
  {"CHAR", "CHAR(255)", sql::DataType::CHAR, "abc", true, 255, 0, true, "NULL", 0, "NO", false},
  {"BINARY", "CHAR(255) CHARACTER SET binary", sql::DataType::BINARY, "abc", true, 255, 0, true, "NULL", 0, "NO", false},
  {"CHAR", "CHAR(254) NOT NULL", sql::DataType::CHAR, "abc", true, 254, 0, false, "", 0, "NO", false},
  {"CHAR", "CHAR(254) NOT NULL DEFAULT 'abc'", sql::DataType::CHAR, "abc", true, 254, 0, false, "'abc'", 0, "NO", false},
  // 3byte

  {"CHAR", "NATIONAL CHAR(255)", sql::DataType::CHAR, "abc", true, 255, 0, true, "NULL", 765, "NO", false},
  {"CHAR", "NATIONAL CHAR(215) NOT NULL", sql::DataType::CHAR, "abc", true, 215, 0, false, "", 645, "NO", false},
  {"CHAR", "NATIONAL CHAR(215) NOT NULL DEFAULT 'Ulf'", sql::DataType::CHAR, "abc", true, 215, 0, false, "'Ulf'", 645, "NO", false},
  {"CHAR", "CHAR(255) CHARACTER SET 'utf8'", sql::DataType::CHAR, "abc", true, 255, 0, true, "NULL", 765, "NO", false},
  {"CHAR", "CHAR(55) CHARACTER SET 'utf8' NOT NULL ", sql::DataType::CHAR, "abc", true, 55, 0, false, "", 165, "NO", false},
  // TODO this might be server dependent!
  {"CHAR", "CHAR(250) CHARACTER SET 'utf8' COLLATE 'utf8_bin'", sql::DataType::CHAR, "abc", true, 250, 0, true, "NULL", 750, "NO", false},
  {"CHAR", "CHAR(250) CHARACTER SET 'utf8' COLLATE 'utf8_bin' DEFAULT 'Wendel'", sql::DataType::CHAR, "abc", true, 250, 0, true, "'Wendel'", 750, "NO", false},
  {"CHAR", "CHAR(43) CHARACTER SET 'utf8' COLLATE 'utf8_bin' NOT NULL", sql::DataType::CHAR, "'abc'", true, 43, 0, false, "", 129, "NO", false},
  {"CHAR", "CHAR(123) CHARACTER SET 'ucs2'", sql::DataType::CHAR, "abc", true, 123, 0, true, "NULL", 246, "NO", false},
  // The CHAR BYTE data type is an alias for the BINARY data type. This is a compatibility feature.
  {"BINARY", "CHAR(255) BYTE", sql::DataType::BINARY, "abc", true, 255, 0, true, "NULL", 255, "NO", false},
  {"BINARY", "CHAR(12) BYTE NOT NULL", sql::DataType::BINARY, "abc", true, 12, 0, false, "", 12, "NO", false},
  {"CHAR", "CHAR(14) DEFAULT 'Andrey'", sql::DataType::CHAR, "abc", true, 14, 0, true, "'Andrey'", 0, "NO", false},
  {"BINARY", "CHAR(25) CHARACTER SET 'binary'", sql::DataType::BINARY, "abc", true, 25, 0, true, "NULL", 25, "NO", false},
  {"VARCHAR", "VARCHAR(10)", sql::DataType::VARCHAR, "a", true, 10, 0, true, "NULL", 0, "NO", false},
  {"VARBINARY", "VARCHAR(10) CHARACTER SET binary", sql::DataType::VARBINARY, "a", true, 10, 0, true, "NULL", 0, "NO", false},
  {"VARCHAR", "VARCHAR(7) NOT NULL", sql::DataType::VARCHAR, "a", true, 7, 0, false, "", 0, "NO", false},
  {"VARCHAR", "VARCHAR(255) DEFAULT 'Good night twitter. BTW, go MySQL!'", sql::DataType::VARCHAR, "a", true, 255, 0, true, "'Good night twitter. BTW, go MySQL!'", 0, "NO", false},
  {"VARCHAR", "VARCHAR(11) CHARACTER SET 'utf8'", sql::DataType::VARCHAR, "a", true, 11, 0, true, "NULL", 33, "NO", false},
  {"VARCHAR", "VARCHAR(11) CHARACTER SET 'ascii' DEFAULT 'Hristov'", sql::DataType::VARCHAR, "a", true, 11, 0, true, "'Hristov'", 11, "NO", false},
  {"VARCHAR", "VARCHAR(12) CHARACTER SET 'utf8' COLLATE 'utf8_bin'", sql::DataType::VARCHAR, "a", true, 12, 0, true, "NULL", 36, "NO", false},
  {"VARBINARY", "VARCHAR(13) BYTE", sql::DataType::VARBINARY, "a", true, 13, 0, true, "NULL", 13, "NO", false},
  {"VARBINARY", "VARCHAR(14) BYTE NOT NULL", sql::DataType::VARBINARY, "a", true, 14, 0, false, "", 14, "NO", false},
  {"BINARY", "BINARY(1)", sql::DataType::BINARY, "a", true, 1, 0, true, "NULL", 1, "NO", false},
  {"VARBINARY", "VARBINARY(1)", sql::DataType::VARBINARY, "a", true, 1, 0, true, "NULL", 1, "NO", false},
  {"VARBINARY", "VARBINARY(2) NOT NULL", sql::DataType::VARBINARY, "a", true, 2, 0, false, "", 2, "NO", false},
  {"VARBINARY", "VARBINARY(20) NOT NULL DEFAULT 0x4C617772696E", sql::DataType::VARBINARY, "a", true, 20, 0, false, "'Lawrin'", 20, "NO", false},
  {"TINYBLOB", "TINYBLOB", sql::DataType::VARBINARY, "a", true, 255, 0, true, "NULL", 255, "NO", false},
  {"TINYTEXT", "TINYTEXT", sql::DataType::VARCHAR, "a", true, 255, 0, true, "NULL", 255, "NO", false, "a"},
  {"TINYTEXT", "TINYTEXT NOT NULL", sql::DataType::VARCHAR, "a", true, 255, 0, false, "", 255, "NO", false},
  {"TINYTEXT", "TINYTEXT", sql::DataType::VARCHAR, "a", true, 255, 0, true, "NULL", 255, "NO", false},
  {"TINYBLOB", "TINYTEXT CHARACTER SET binary", sql::DataType::VARBINARY, "a", true, 255, 0, true, "NULL", 255, "NO", false},
  {"TINYTEXT", "TINYTEXT CHARACTER SET 'utf8'", sql::DataType::VARCHAR, "a", true, 255, 0, true, "NULL", 255, "NO", false},
  {"TINYTEXT", "TINYTEXT CHARACTER SET 'utf8' COLLATE 'utf8_bin'", sql::DataType::VARCHAR, "a", true, 255, 0, true, "NULL", 255, "NO", false},
  {"BLOB", "BLOB", sql::DataType::LONGVARBINARY, "a", true, 65535, 0, true, "NULL", 65536, "NO", false},
  {"BLOB", "BLOB NOT NULL", sql::DataType::LONGVARBINARY, "a", true, 65535, 0, false, "", 65535, "NO", false},
  {"TEXT", "TEXT", sql::DataType::LONGVARCHAR, "a", true, 65535, 0, true, "NULL", 65536, "NO", false},
  {"TEXT", "TEXT NOT NULL", sql::DataType::LONGVARCHAR, "a", true, 65535, 0, false, "", 65535, "NO", false},
  {"MEDIUMBLOB", "MEDIUMBLOB", sql::DataType::LONGVARBINARY, "a", true, 16777215, 0, true, "NULL", 16777215, "NO", false},
  {"MEDIUMBLOB", "MEDIUMBLOB NOT NULL", sql::DataType::LONGVARBINARY, "a", true, 16777215, 0, false, "", 16777215, "NO", false},
  {"MEDIUMTEXT", "MEDIUMTEXT", sql::DataType::LONGVARCHAR, "a", true, 16777215, 0, true, "NULL", 16777215, "NO", false},
  {"MEDIUMBLOB", "MEDIUMTEXT CHARACTER SET binary", sql::DataType::LONGVARBINARY, "a", true, 16777215, 0, true, "NULL", 16777215, "NO", false},
  {"MEDIUMTEXT", "MEDIUMTEXT NOT NULL", sql::DataType::LONGVARCHAR, "a", true, 16777215, 0, false, "", 16777215, "NO", false},
  {"MEDIUMTEXT", "MEDIUMTEXT CHARSET 'utf8'", sql::DataType::LONGVARCHAR, "a", true, 16777215, 0, true, "NULL", 16777215, "NO", false},
  {"MEDIUMTEXT", "MEDIUMTEXT CHARSET 'utf8' COLLATE 'utf8_bin'", sql::DataType::LONGVARCHAR, "a", true, 16777215, 0, true, "NULL", 16777215, "NO", false},
  {"LONGBLOB", "LONGBLOB", sql::DataType::LONGVARBINARY, "a", true, L64(4294967295), 0, true, "NULL", L64(4294967295), "NO", false},
  {"LONGBLOB", "LONGBLOB NOT NULL", sql::DataType::LONGVARBINARY, "a", true, L64(4294967295), 0, false, "", L64(4294967295), "NO", false},
  {"LONGTEXT", "LONGTEXT", sql::DataType::LONGVARCHAR, "a", true, L64(4294967295), 0, true, "NULL", L64(4294967295), "NO", false},
  {"LONGBLOB", "LONGTEXT CHARACTER SET binary", sql::DataType::LONGVARBINARY, "a", true, L64(4294967295), 0, true, "NULL", L64(4294967295), "NO", false},
  {"LONGTEXT", "LONGTEXT NOT NULL", sql::DataType::LONGVARCHAR, "a", true, L64(4294967295), 0, false, "", L64(4294967295), "NO", false},
  {"LONGTEXT", "LONGTEXT CHARSET 'utf8'", sql::DataType::LONGVARCHAR, "a", true, L64(4294967295), 0, true, "NULL", L64(4294967295), "NO", false},
  {"LONGTEXT", "LONGTEXT CHARSET 'utf8' COLLATE 'utf8_bin'", sql::DataType::LONGVARCHAR, "a", true, L64(4294967295), 0, true, "NULL", L64(4294967295), "NO", false},
  {"ENUM", "ENUM('yes', 'no')", sql::DataType::VARCHAR, "yes", true, 3, 0, true, "NULL", 3, "NO", false},
  {"ENUM", "ENUM('yes', 'no') CHARACTER SET 'binary'", sql::DataType::VARCHAR, "yes", true, 3, 0, true, "NULL", 3, "NO", false},
  {"ENUM", "ENUM('yes', 'no') NOT NULL", sql::DataType::VARCHAR, "yes", true, 3, 0, false, "", 3, "NO", false, "yes"},
  {"ENUM", "ENUM('yes', 'no', 'not sure') NOT NULL", sql::DataType::VARCHAR, "yes", true, 8, 0, false, "", 8, "NO", false},
  {"ENUM", "ENUM('yes', 'no', 'buy') NOT NULL DEFAULT 'buy'", sql::DataType::VARCHAR, "yes", true, 3, 0, false, "'buy'", 3, "NO", false},
  {"SET", "SET('yes', 'no')", sql::DataType::VARCHAR, "yes", true, 6, 0, true, "NULL", 6, "NO", false, "yes"},
  {"SET", "SET('yes', 'no') CHARACTER SET 'binary'", sql::DataType::VARCHAR, "yes", true, 6, 0, true, "NULL", 6, "NO", false, "yes"},
  {"SET", "SET('yes', 'no') CHARSET 'ascii'", sql::DataType::VARCHAR, "yes", true, 6, 0, true, "NULL", 6, "NO", false},
  {"SET", "SET('yes', 'no') CHARSET 'ascii' DEFAULT 'yes'", sql::DataType::VARCHAR, "yes", true, 6, 0, true, "'yes'", 6, "NO", false},
  {"SET", "SET('yes', 'no', 'ascii') CHARSET 'ascii' NOT NULL", sql::DataType::VARCHAR, "yes", true, 12, 0, false, "", 12, "NO", false}
};

/*
ResultSet getAttributes(String catalog,
                  String schemaPattern,
                  String typeNamePattern,
                  String attributeNamePattern)
                  throws SQLException

Retrieves a description of the given attribute of the given type for a user-defined type (UDT) that is available in the given schema and catalog.

Descriptions are returned only for attributes of UDTs matching the catalog, schema, type, and attribute name criteria. They are ordered by TYPE_CAT, TYPE_SCHEM, TYPE_NAME and ORDINAL_POSITION. This description does not contain inherited attributes.

The ResultSet object that is returned has the following columns:

  1. TYPE_CAT String => type catalog (may be null)
  2. TYPE_SCHEM String => type schema (may be null)
  3. TYPE_NAME String => type name
  4. ATTR_NAME String => attribute name
  5. DATA_TYPE int => attribute type SQL type from java.sql.Types
  6. ATTR_TYPE_NAME String => Data source dependent type name. For a UDT, the type name is fully qualified. For a REF, the type name is fully qualified and represents the target type of the reference type.
  7. ATTR_SIZE int => column size. For char or date types this is the maximum number of characters; for numeric or decimal types this is precision.
  8. DECIMAL_DIGITS int => the number of fractional digits. Null is returned for data types where DECIMAL_DIGITS is not applicable.
  9. NUM_PREC_RADIX int => Radix (typically either 10 or 2)
  10. NULLABLE int => whether NULL is allowed
   * attributeNoNulls - might not allow NULL values
   * attributeNullable - definitely allows NULL values
   * attributeNullableUnknown - nullability unknown
  11. REMARKS String => comment describing column (may be null)
  12. ATTR_DEF String => default value (may be null)
  13. SQL_DATA_TYPE int => unused
  14. SQL_DATETIME_SUB int => unused
  15. CHAR_OCTET_LENGTH int => for char types the maximum number of bytes in the column
  16. ORDINAL_POSITION int => index of the attribute in the UDT (starting at 1)
  17. IS_NULLABLE String => ISO rules are used to determine the nullability for a attribute.
   * YES --- if the attribute can include NULLs
   * NO --- if the attribute cannot include NULLs
   * empty string --- if the nullability for the attribute is unknown
  18. SCOPE_CATALOG String => catalog of table that is the scope of a reference attribute (null if DATA_TYPE isn't REF)
  19. SCOPE_SCHEMA String => schema of table that is the scope of a reference attribute (null if DATA_TYPE isn't REF)
  20. SCOPE_TABLE String => table name that is the scope of a reference attribute (null if the DATA_TYPE isn't REF)
  21. SOURCE_DATA_TYPE short => source type of a distinct type or user-generated Ref type,SQL type from java.sql.Types (null if DATA_TYPE isn't DISTINCT or user-generated REF)
   */
std::vector< udtattribute > unit_fixture::attributes = {
  {"TYPE_CAT", 0},
  {"TYPE_SCHEM", 0},
  {"TYPE_NAME", 0},
  {"ATTR_NAME", 0},
  {"DATA_TYPE", 0},
  {"ATTR_TYPE_NAME", 0},
  {"ATTR_SIZE", 0},
  {"DECIMAL_DIGITS", 0},
  {"NUM_PREC_RADIX", 0},
  {"NULLABLE", 0},
  {"REMARKS", 0},
  {"ATTR_DEF", 0},
  {"SQL_DATA_TYPE", 0},
  {"SQL_DATETIME_SUB", 0},
  {"CHAR_OCTET_LENGTH", 0},
  {"ORDINAL_POSITION", 0},
  {"IS_NULLABLE", 0},
  {"SCOPE_CATALOG", 0},
  {"SCOPE_SCHEMA", 0},
  {"SCOPE_TABLE", 0},
  {"SOURCE_DATA_TYPE", 0},
};

unit_fixture::unit_fixture(const String & name)
: super(name),
con(nullptr),
pstmt(nullptr),
stmt(nullptr),
res(nullptr),
useTls(false)
{
  init();
}


void unit_fixture::init()
{
  url= TestsRunner::theInstance().getStartOptions()->getString("dbUrl");
  user= TestsRunner::theInstance().getStartOptions()->getString("dbUser");
  passwd= TestsRunner::theInstance().getStartOptions()->getString("dbPasswd");
  db= TestsRunner::theInstance().getStartOptions()->getString("dbSchema");
  useTls= TestsRunner::theInstance().getStartOptions()->getBool("useTls");

  // Normalizing URL
  std::size_t protocolEnd = url.find("://"); //5 - length of jdbc: prefix

  if (protocolEnd == std::string::npos)
  {
    url= "jdbc:mariadb://" + url;
  }
  else 
  {
    sql::SQLString protocol(url.substr(0, protocolEnd));

    if (protocol.compare("jdbc:mariadb:") != 0)
    {
      url = url.substr(protocolEnd + 3/*://*/);
      std::size_t slashPos= url.find_first_of('/');
      sql::SQLString hostName(slashPos == std::string::npos ? url : url.substr(0, slashPos));

      url= "jdbc:mariadb://" + url;
      if (protocol.compare("tcp") == 0)
      {
        // Seems like there is nothing to do here
      }
      else if (protocol.compare("unix") == 0)
      {
        commonProperties["localSocket"]= hostName;
      }
      else if (protocol.compare("pipe") == 0)
      {
        commonProperties["pipe"]= hostName;
      }
    }
  }
  // Now we are supposed to have jdbc:mariadb:// prefix
  std::size_t slashPos = url.find_first_of('/', sizeof("jdbc:mariadb://"));

  if (slashPos == std::string::npos)
  {
    urlWithoutSchema= url;
    url.reserve(url.length() + 1 + db.length());
    url.append("/").append(db);
  }
  else
  {
    urlWithoutSchema= url.substr(0, slashPos);
  }
}


void unit_fixture::setUp()
{
  created_objects.clear();
  undo.clear();

  if (!con) {
    try
    {
      con.reset(this->getConnection(&commonProperties));
    }
    catch (sql::SQLException& sqle)
    {
      logErr(String("Couldn't get connection") + sqle.what());
      logDebug("Host:" + url + ", UID:" + user + ", Schema:" + db + ", Tls:" + (useTls ? "yes" : "no")/* + ", Pwd:" + passwd + "<<<"*/);
      for (auto& cit : commonProperties) {
        String property(cit.first.c_str());
        logDebug(property + ":" + cit.second.c_str());
      }
      throw sqle;
    }
  }
  logDebug("Host: " + url + ", UID: " + user + ", Schema: " + db + ", Tls: " + (useTls ? "yes" : "no"));

  /*
   logDebug("Driver: " + driver->getName());
           + " " + String(driver->getMajorVersion() + driver->getMajorVersion + String(".") + driver->getMinorVersion());*/

  //con->setSchema(db);

  stmt.reset(con->createStatement());
}

void unit_fixture::tearDown()
{
  res.reset();
  for (int i=0; i < static_cast<int> (created_objects.size() - 1); i+=2)
  {
    try
    {
      dropSchemaObject(created_objects[ i ], created_objects[ i + 1 ]);
    }
    catch (sql::SQLException &)
    {
    }
  }

  if (!undo.empty()) {
    std::ostringstream undoCombined(undo.front().c_str(), std::ios_base::ate);
    undo.pop_front();
    for (auto& undoQuery : undo) {
      undoCombined << ";" << undoQuery;
    }
    stmt->execute(undoCombined.str());
  }

  stmt.reset();
  pstmt.reset();
  cstmt.reset();
  if (con) {
    if (con->isClosed()) {
      // Resetting the pointer, i.e.destructing if it is closed. We can't reset it and make usable
      con.reset();
    }
    else {
      // resetting the connection for future use
      con->reset();
    }
  }
}

void unit_fixture::createSchemaObject(String object_type, String object_name,
                                      String columns_and_other_stuff)
{
  created_objects.push_back(object_type);
  created_objects.push_back(object_name);

  dropSchemaObject(object_type, object_name);

  String sql("CREATE  ");

  sql.append(object_type);
  sql.append(" ");
  sql.append(object_name);
  sql.append(" ");
  sql.append(columns_and_other_stuff);

  stmt->executeUpdate(sql);
}

void unit_fixture::dropSchemaObject(String object_type, String object_name)
{
  stmt->executeUpdate(String("DROP ") + object_type + " IF EXISTS "
                      + object_name);
}

void unit_fixture::createTable(String table_name, String columns_and_other_stuff)
{
  createSchemaObject("TABLE", table_name, columns_and_other_stuff);
}


void unit_fixture::dropTable(String table_name)
{
  dropSchemaObject("TABLE", table_name);
}


sql::Connection *
unit_fixture::getConnection(sql::ConnectOptionsMap *additional_options)
{
  if (driver == NULL)
  {
    driver=sql::mariadb::get_driver_instance();
  }

  sql::ConnectOptionsMap connection_properties;
  connection_properties["user"]= user;
  connection_properties["password"]= passwd;

  connection_properties["useTls"]= useTls ? "true" : "false";

  bool bval= !TestsRunner::getStartOptions()->getBool("dont-use-is");
  connection_properties["metadataUseInfoSchema"]= bval ? "1" : "0";

  if (additional_options != nullptr)
  {
    for (sql::ConnectOptionsMap::const_iterator cit= additional_options->cbegin();
         cit != additional_options->cend(); ++cit)
    {
      connection_properties[cit->first]= cit->second;
    }
  }

  return sql::DriverManager::getConnection(url, connection_properties);
}

void unit_fixture::logMsg(const sql::SQLString & message)
{
  TestsListener::messagesLog(message + "\n");
}

void unit_fixture::logErr(const sql::SQLString & message)
{
  TestsListener::errorsLog(message + "\n");
}


void unit_fixture::logDebug(const String & message)
{
  logMsg(message);
}


/* There is not really need to have it as a class method */
int unit_fixture::getServerVersion(Connection & con)
{
  DatabaseMetaData dbmeta(con->getMetaData());
  return dbmeta->getDatabaseMajorVersion() * 100000 + dbmeta->getDatabaseMinorVersion() * 1000 + dbmeta->getDatabasePatchVersion();
}

sql::SQLString unit_fixture::getVariableValue(const sql::SQLString& name, bool global)
{
  res.reset(stmt->executeQuery((global ? "SELECT @@global." : "SELECT @@") + name));
  res->next();
  return res->getString(1);
}

bool unit_fixture::setVariableValue(const sql::SQLString& name, const sql::SQLString& value, bool global)
{
  sql::SQLString currentValue(getVariableValue(name), global);

  if (value.compare(currentValue) != 0) {
    if (global) {
      stmt->execute("SET GLOBAL " + name + "=" + value);
      undo.push_front("SET GLOBAL " + name + "=" + currentValue);
    }
    else {
      stmt->execute("SET SESSION " + name + "=" + value);
    }
  }
  return false;
}

std::string unit_fixture::exceptionIsOK(sql::SQLException & e)
{
  return exceptionIsOK(e, "HY000", 0);
}

std::string unit_fixture::exceptionIsOK(sql::SQLException &e, const std::string& sql_state, int errNo)
{

  std::stringstream reason;
  reason.str("");

  std::string what(e.what());
  if (what.empty())
  {
    reason << "Exception must not have an empty message.";
    logMsg(reason.str());
    return reason.str();
  }

  if (e.getErrorCode() != errNo)
  {
    reason << "Expecting error code '" << errNo << "' got '" << e.getErrorCode() << "'";
    logMsg(reason.str());
    return reason.str();
  }

  if (e.getSQLState() != sql_state)
  {
    reason << "Expecting sqlstate '" << sql_state << "' got '" << e.getSQLState() << "'";
    logMsg(reason.str());
    return reason.str();
  }

  return reason.str();
}

void unit_fixture::checkResultSetScrolling(ResultSet &res_ref)
{
  if (res_ref->getType() == sql::ResultSet::TYPE_FORWARD_ONLY) {
    return;
  }

  int before;

  before=static_cast<int> (res_ref->getRow());
  if (!res_ref->last())
  {
    res_ref->absolute(before);
    return;
  }

  int num_rows;
  int i;

  num_rows=(int) res_ref->getRow();

  res_ref->beforeFirst();

  ASSERT(!res_ref->previous());
  ASSERT(res_ref->next());
  ASSERT_EQUALS(1, (int) res_ref->getRow());
  ASSERT(!res_ref->isBeforeFirst());
  ASSERT(!res_ref->isAfterLast());
  if (num_rows > 1)
  {
    ASSERT(res_ref->next());
    ASSERT(res_ref->previous());
    ASSERT_EQUALS(1, (int) res_ref->getRow());
  }

  ASSERT(res_ref->first());
  ASSERT_EQUALS(1, (int) res_ref->getRow());
  ASSERT(res_ref->isFirst());
  ASSERT(!res_ref->isBeforeFirst());
  ASSERT(!res_ref->isAfterLast());
  if (num_rows == 1)
    ASSERT(res_ref->isLast());
  else
    ASSERT(!res_ref->isLast());

  if (num_rows > 1)
  {
    ASSERT(res_ref->next());
    ASSERT(res_ref->previous());
    ASSERT_EQUALS(1, (int) res_ref->getRow());
  }
  ASSERT(!res_ref->previous());

  ASSERT(res_ref->last());
  ASSERT_EQUALS(num_rows, (int) res_ref->getRow());
  ASSERT(res_ref->isLast());
  ASSERT(!res_ref->isBeforeFirst());
  ASSERT(!res_ref->isAfterLast());
  if (num_rows == 1)
    ASSERT(res_ref->isFirst());
  else
    ASSERT(!res_ref->isFirst());

  if (num_rows > 1)
  {
    ASSERT(res_ref->previous());
    ASSERT_EQUALS(num_rows - 1, (int) res_ref->getRow());
    ASSERT(res_ref->next());
    ASSERT_EQUALS(num_rows, (int) res_ref->getRow());
  }
  ASSERT(!res_ref->next());

  res_ref->beforeFirst();
  ASSERT_EQUALS(0, (int) res_ref->getRow());
  ASSERT(res_ref->isBeforeFirst());
  ASSERT(!res_ref->isAfterLast());
  ASSERT(!res_ref->isFirst());
  ASSERT(!res_ref->previous());
  ASSERT(res_ref->next());
  ASSERT_EQUALS(1, (int) res_ref->getRow());
  res_ref->absolute(1);
  ASSERT_EQUALS(1, (int) res_ref->getRow());
  ASSERT(!res_ref->previous());

  res_ref->afterLast();
  ASSERT_EQUALS(num_rows + 1, (int) res_ref->getRow());
  ASSERT(res_ref->isAfterLast());
  ASSERT(!res_ref->isBeforeFirst());
  ASSERT(!res_ref->isFirst());
  ASSERT(res_ref->previous());
  ASSERT_EQUALS(num_rows, (int) res_ref->getRow());
  ASSERT(!res_ref->next());

  i=0;
  res_ref->beforeFirst();
  while (res_ref->next())
  {
    ASSERT(!res_ref->isAfterLast());
    i++;
    ASSERT_EQUALS(i, (int) res_ref->getRow());
  }
  ASSERT_EQUALS(num_rows, i);

  // relative(1) is equivalent to next()
  i=0;
  res_ref->beforeFirst();
  while (res_ref->relative(1))
  {
    ASSERT(!res_ref->isAfterLast());
    i++;
    ASSERT_EQUALS(i, (int) res_ref->getRow());
  }
  ASSERT_EQUALS(num_rows, i);

  i=0;
  res_ref->first();
  do
  {
    ASSERT(!res_ref->isAfterLast());
    i++;
    ASSERT_EQUALS(i, (int) res_ref->getRow());
  }
  while (res_ref->next());
  ASSERT_EQUALS(num_rows, i);

  // relative(1) is equivalent to next()
  i=0;
  res_ref->first();
  do
  {
    ASSERT(!res_ref->isAfterLast());
    i++;
    ASSERT_EQUALS(i, (int) res_ref->getRow());
  }
  while (res_ref->relative(1));
  ASSERT_EQUALS(num_rows, i);

  i=num_rows;
  res_ref->last();
  do
  {
    ASSERT(!res_ref->isBeforeFirst());
    ASSERT_EQUALS(i, (int) res_ref->getRow());
    i--;
  }
  while (res_ref->previous());
  ASSERT_EQUALS(0, i);

  // relative(-1) is equivalent to previous()
  i=num_rows;
  res_ref->last();
  do
  {
    ASSERT(!res_ref->isBeforeFirst());
    ASSERT_EQUALS(i, (int) res_ref->getRow());
    i--;
  }
  while (res_ref->relative(-1));
  ASSERT_EQUALS(0, i);

  i=num_rows;
  res_ref->afterLast();
  while (res_ref->previous())
  {
    ASSERT(!res_ref->isBeforeFirst());
    ASSERT_EQUALS(i, (int) res_ref->getRow());
    i--;
  }
  ASSERT_EQUALS(0, i);

  // relative(-1) is equivalent to previous()

  i=num_rows;
  res_ref->afterLast();
  while (res_ref->relative(-1))
  {
    ASSERT(!res_ref->isBeforeFirst());
    ASSERT_EQUALS(i, (int) res_ref->getRow());
    i--;
  }
  ASSERT_EQUALS(0, i);

  res_ref->last();
  res_ref->relative(0);
  ASSERT(res_ref->isLast());

  res_ref->absolute(before);
  ASSERT_EQUALS(before, (int) res_ref->getRow());
}


bool unit_fixture::isSkySqlHA() const
{
  static bool SkySqlHAinTravis= (std::getenv("SKYSQL_HA") != nullptr);
  return SkySqlHAinTravis;
}


bool unit_fixture::isMaxScale() const
{
  static bool MaxScaleOnTravis= (std::getenv("MAXSCALE_TEST_DISABLE") != nullptr);
  return MaxScaleOnTravis;
}
} /* namespace testsuite */
