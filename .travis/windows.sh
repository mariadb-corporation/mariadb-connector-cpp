#!/bin/bash

set -x
set -e

###################################################################################################################
# test different type of configuration
###################################################################################################################

export TEST_SOCKET=
export ENTRYPOINT=$PROJ_PATH/.travis/sql
export ENTRYPOINT_PAM=$PROJ_PATH/.travis/pam

set +x

if [ -n "$SKYSQL" ] || [ -n "$SKYSQL_HA" ]; then
  if [ -n "$SKYSQL" ]; then
    ###################################################################################################################
    # test SKYSQL
    ###################################################################################################################
    if [ -z "$SKYSQL_HOST" ] ; then
      echo "No SkySQL configuration found !"
      exit 0
    fi

    export TEST_UID=$SKYSQL_USER
    export TEST_SERVER=$SKYSQL_HOST
    export TEST_PASSWORD=$SKYSQL_PASSWORD
    export TEST_PORT=$SKYSQL_PORT
    export TEST_SCHEMA=testcpp
    export TEST_USETLS=true

  else

    ###################################################################################################################
    # test SKYSQL with replication
    ###################################################################################################################
    if [ -z "$SKYSQL_HA" ] ; then
      echo "No SkySQL HA configuration found !"
      exit 0
    fi

    export TEST_UID=$SKYSQL_HA_USER
    export TEST_SERVER=$SKYSQL_HA_HOST
    export TEST_PASSWORD=$SKYSQL_HA_PASSWORD
    export TEST_PORT=$SKYSQL_HA_PORT
    export TEST_SCHEMA=testcpp
    export TEST_USETLS=true
  fi

else

  export TEST_SERVER=localhost
  export TEST_SCHEMA=testcpp
  export TEST_UID=root
  export TEST_PASSWORD=
  export TEST_PORT=3306

  set -x
  ls /c/Program\ Files/
  MDBPATH=$(find /c/Program\ Files/MariaDB\ 1* -maxdepth 0 | tail -1)
  PATH=$PATH:$MDBPATH/bin

  if [-z "$WIX"] ; then
    echo "WIX was not set at installation time"
  fi

  # create test database
  mariadb -e "DROP SCHEMA IF EXISTS $TEST_SCHEMA" --user=$TEST_UID --password="$TEST_PASSWORD"
  mariadb -e "CREATE DATABASE $TEST_SCHEMA" --user=$TEST_UID --password="$TEST_PASSWORD"

  mariadb=( mariadb --protocol=TCP --user=${TEST_UID} -h${TEST_SERVER} --port=${TEST_PORT} ${TEST_SCHEMA})

  #list ssl certificates
  ls -lrt ${SSLCERT}

fi

cmake -DCONC_WITH_MSI=OFF -DCONC_WITH_UNIT_TESTS=OFF -DWITH_MSI=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_SSL=SCHANNEL .
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
#ls -l cts.sql
#
#if [ -n "$SKYSQL" ] ; then
#  set +x
#  mysql --protocol=tcp -u$TEST_UID -h$TEST_SERVER --port=$TEST_PORT --ssl -p$TEST_PASSWORD < cts.sql
#  set -x
#else
#  mysql --protocol=tcp -u$TEST_UID -h$TEST_SERVER --port=$TEST_PORT --password=$TEST_PASSWORD < cts.sql
#fi

ctest --output-on-failure

