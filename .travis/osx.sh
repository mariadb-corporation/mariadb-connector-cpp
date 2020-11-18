#!/bin/bash

set -x
set -e

export TEST_SERVER=mariadb.example.com
export TEST_PORT=3305
export TEST_SOCKET=
export TEST_SCHEMA=test
export TEST_UID=bob
export TEST_PASSWORD= 

# mysql.server start

# ls -la /usr/local/Cellar/openssl/
cmake -DCONC_WITH_MSI=OFF -DCONC_WITH_UNIT_TESTS=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_OPENSSL=ON -DWITH_SSL=OPENSSL -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib -DTEST_DEFAULT_HOST="tcp://mariadb.example.com:$TEST_PORT" -DTEST_DEFAULT_DB=test -DTEST_DEFAULT_LOGIN=bob -DTEST_DEFAULT_PASSWD="" .
cmake --build . --config RelWithDebInfo

###################################################################################################################
# run test suite
###################################################################################################################

# check users of MariaDB and create test database
#mysql --version
#mysql -u bob -e "CREATE DATABASE test"
#mysql -u bob -e "SHOW DATABASES"

echo "Running tests"
cd test
mysql --protocol=tcp -ubob -h127.0.0.1 --port=$TEST_PORT < cts.sql
ctest -VV
