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


export TEST_SOCKET=

set +x
if [ -n "$SKYSQL" ] ; then
  if [ -z "$SKYSQL_TEST_HOST" ] ; then
    echo "No SkySQL configuration found !"
    exit 0
  fi
  #&useSsl&serverSslCert=$SKYSQL_TEST_SSL_CA"

  export TEST_SERVER=$SKYSQL_TEST_HOST
  export TEST_SCHEMA=testcpp
  export TEST_UID=$SKYSQL_TEST_USER
  export TEST_PASSWORD=$SKYSQL_TEST_PASSWORD
  TEST_PORT=$SKYSQL_TEST_PORT
  export TEST_USETLS=true

else
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
  export TEST_SCHEMA=test
  export TEST_UID=bob
  export TEST_PASSWORD= 
fi

cmake -DCONC_WITH_MSI=OFF -DCONC_WITH_UNIT_TESTS=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_OPENSSL=ON -DWITH_SSL=OPENSSL -DTEST_HOST="tcp://$TEST_SERVER:$TEST_PORT" .
# In Travis we are interested in tests with latest C/C version, while for release we must use only latest release tag
#git submodule update --remote
set -x
cmake --build . --config RelWithDebInfo 

###################################################################################################################
# run test suite
###################################################################################################################
echo "Running tests"

cd test

pwd
ls -l cts.sql

if [ -n "$SKYSQL" ] ; then
  set +x
  mysql --protocol=tcp -u$TEST_UID -h$TEST_SERVER --port=$TEST_PORT --ssl -p$TEST_PASSWORD < cts.sql
  set -x
else
  mysql --protocol=tcp -u$TEST_UID -h$TEST_SERVER --port=$TEST_PORT --password=$TEST_PASSWORD < cts.sql
fi

ctest -VV

