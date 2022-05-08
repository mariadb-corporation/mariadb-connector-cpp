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
  export COMPOSE_FILE=.travis/docker-compose.yml

  export TEST_SERVER=mariadb.example.com
  export TEST_SCHEMA=testcpp
  export TEST_UID=bob
  export TEST_PASSWORD=
  export TEST_PORT=3305

  if [ -n "$MAXSCALE_VERSION" ] ; then
      # maxscale ports:
      # - non ssl: 4006
      # - ssl: 4009
      export TEST_PORT=4006
      export TEST_SSL_PORT=4009
      export COMPOSE_FILE=.travis/maxscale-compose.yml
      docker-compose -f ${COMPOSE_FILE} build
  fi

  mysql=( mysql --protocol=TCP -u${TEST_UID} -h${TEST_SERVER} --port=${TEST_PORT} ${TEST_SCHEMA})


  ###################################################################################################################
  # launch docker server and maxscale
  ###################################################################################################################
  docker-compose -f ${COMPOSE_FILE} up -d

  ###################################################################################################################
  # wait for docker initialisation
  ###################################################################################################################

  for i in {30..0}; do
    if echo 'SELECT 1' | "${mysql[@]}" &> /dev/null; then
        break
    fi
    echo 'data server still not active'
    sleep 2
  done

  if [ "$i" = 0 ]; then
    if echo 'SELECT 1' | "${mysql[@]}" ; then
        break
    fi

    docker-compose -f ${COMPOSE_FILE} logs
    if [ -n "$MAXSCALE_VERSION" ] ; then
        docker-compose -f ${COMPOSE_FILE} exec maxscale tail -n 500 /var/log/maxscale/maxscale.log
    fi
    echo >&2 'data server init process failed.'
    exit 1
  fi

  if [[ "$DB" != mysql* ]] ; then
    ###################################################################################################################
    # execute pam
    ###################################################################################################################
    docker-compose -f ${COMPOSE_FILE} exec -u root db bash /pam/pam.sh
    sleep 1
    docker-compose -f ${COMPOSE_FILE} restart db
    sleep 5

    ###################################################################################################################
    # wait for restart
    ###################################################################################################################

    for i in {30..0}; do
      if echo 'SELECT 1' | "${mysql[@]}" &> /dev/null; then
          break
      fi
      echo 'data server restart still not active'
      sleep 2
    done

    if [ "$i" = 0 ]; then
      if echo 'SELECT 1' | "${mysql[@]}" ; then
          break
      fi

      docker-compose -f ${COMPOSE_FILE} logs
      if [ -n "$MAXSCALE_VERSION" ] ; then
          docker-compose -f ${COMPOSE_FILE} exec maxscale tail -n 500 /var/log/maxscale/maxscale.log
      fi
      echo >&2 'data server restart process failed.'
      exit 1
    fi
  fi

  #list ssl certificates
  ls -lrt ${SSLCERT}

fi

cmake -DCONC_WITH_MSI=OFF -DCONC_WITH_UNIT_TESTS=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_SSL=OPENSSL -DTEST_HOST="tcp://$TEST_SERVER:$TEST_PORT" .
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

