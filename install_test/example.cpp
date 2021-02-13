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

#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <sstream>
#include <cmath>

#include "mariadb/conncpp.hpp"

int main(int argc, char** argv)
{
  // Instantiate Driver
  sql::Driver* driver = sql::mariadb::get_driver_instance();

  // Configure Connection
  sql::SQLString url("jdbc:mariadb://localhost/test");
  sql::Properties properties({ {"user", argc > 1 ? argv[1] : "root"}, {"password", argc > 2 ? argv[2] : ""} });

  std::cerr << "Connecting using url: " << url << "with user " << properties["user"] << " and " << properties["password"].length();
  if (argc > 3) {
    properties["localSocket"]= argv[3];
    std::cerr << " via local socket " << argv[3] << std::endl;
  }
  else {
    std::cerr << " via default tcp port" << std::endl;
  }

  // Establish Connection
  try {
    std::unique_ptr<sql::Connection> con(driver->connect(url, properties));

    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery("SELECT 1, 'Hello world'"));
    if (rs->next()) {
     std::cout << rs->getInt(1) << rs->getString(2) << std::endl;
    }
  }
  catch (sql::SQLSyntaxErrorException & e) {
    std::cerr << "[" << e.getSQLState() << "] " << e.what() << "("<< e.getErrorCode() << ")" << std::endl;
  }
  catch (std::regex_error& e) {
    std::cerr << "Regex exception:" << e.what() << std::endl;
  }
  catch (std::exception& e) {
    std::cerr << "Standard exception:" << e.what() << std::endl;
  }

  return 0;
}
