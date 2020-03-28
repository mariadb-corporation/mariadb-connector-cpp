#!/bin/bash

set -x
set -e

export TEST_SERVER=localhost
export TEST_SOCKET=
export TEST_SCHEMA=test
export TEST_UID=root
export TEST_PASSWORD= 

# for some reason brew upgrades postgresql, so let's remove it
# brew remove postgis
# brew uninstall --ignore-dependencies postgresql
# upgrade openssl
# brew upgrade openssl
# install unixodbc
# brew install libiodbc
# install and start MariaDB Server
# brew install mariadb
mysql.server start

# ls -la /usr/local/Cellar/openssl/
cmake -DCONC_WITH_MSI=OFF -DCONC_WITH_UNIT_TESTS=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_OPENSSL=ON -DWITH_SSL=OPENSSL -DOPENSSL_ROOT_DIR=/usr/local/Cellar/openssl/1.0.2o_2 -DOPENSSL_LIBRARIES=/usr/local/Cellar/openssl/1.0.2o_2/lib -DTEST_DEFAULT_HOST="tcp://mariadb.example.com:$TEST_PORT" -DTEST_DEFAULT_DB=test -DTEST_DEFAULT_LOGIN=bob -DTEST_DEFAULT_PASSWD="" .
cmake --build . --config RelWithDebInfo

###################################################################################################################
# run test suite
###################################################################################################################

# check users of MariaDB and create test database
mysql --version
mysql -u root -e "SELECT user, host FROM mysql.user"
mysql -u root -e "CREATE DATABASE test"
mysql -u root -e "SHOW DATABASES"

echo "Running tests"
cd test
mysql --protocol=tcp -ubob -h127.0.0.1 --port=$TEST_PORT < cts.sql
ctest -VV
