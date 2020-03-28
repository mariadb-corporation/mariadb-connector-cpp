#!/bin/bash

set -x
set -e

###################################################################################################################
# test different type of configuration
###################################################################################################################
mysql=( mysql --protocol=tcp -ubob -h127.0.0.1 --port=3305 )
export COMPOSE_FILE=.travis/docker-compose.yml


if [ -n "$MAXSCALE_VERSION" ]
then
  mysql=( mysql --protocol=tcp -ubob -h127.0.0.1 --port=4007 )
  export COMPOSE_FILE=.travis/maxscale-compose.yml
  export TEST_PORT=4007
else
  export TEST_PORT=3305
fi


###################################################################################################################
# launch docker server and maxscale
###################################################################################################################
export INNODB_LOG_FILE_SIZE=$(echo ${PACKET}| cut -d'M' -f 1)0M
docker-compose -f ${COMPOSE_FILE} build
docker-compose -f ${COMPOSE_FILE} up -d


###################################################################################################################
# wait for docker initialisation
###################################################################################################################

for i in {60..0}; do
    if echo 'SELECT 1' | "${mysql[@]}" &> /dev/null; then
        break
    fi
    echo 'data server still not active'
    sleep 1
done

docker-compose -f ${COMPOSE_FILE} logs

if [ "$i" = 0 ]; then
    echo 'SELECT 1' | "${mysql[@]}"
    echo >&2 'data server init process failed.'
    exit 1
fi

#list ssl certificates
ls -lrt ${SSLCERT}


export TEST_SERVER=mariadb.example.com
export TEST_SOCKET=
export TEST_SCHEMA=test
export TEST_UID=bob
export TEST_PASSWORD= 

cmake -DCONC_WITH_MSI=OFF -DCONC_WITH_UNIT_TESTS=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_OPENSSL=ON -DWITH_SSL=OPENSSL -DTEST_DEFAULT_HOST="tcp://mariadb.example.com:$TEST_PORT" -DTEST_DEFAULT_DB=test -DTEST_DEFAULT_LOGIN=bob -DTEST_DEFAULT_PASSWD="" .
# In Travis we are interested in tests with latest C/C version, while for release we must use only latest release tag
#git submodule update --remote
cmake --build . --config RelWithDebInfo 

###################################################################################################################
# run test suite
###################################################################################################################
echo "Running tests"

cd test

mysql --protocol=tcp -ubob -h127.0.0.1 --port=$TEST_PORT < cts.sql
ctest -VV

