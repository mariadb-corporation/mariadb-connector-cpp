#!/bin/bash

set -e

export CCPP_DIR=/home/travis/build/mariadb-corporation/mariadb-connector-cpp
export TEST_UID=$TEST_DB_USER
export TEST_SERVER=$TEST_DB_HOST
export TEST_PASSWORD=$TEST_DB_PASSWORD
export TEST_PORT=$TEST_DB_PORT
export TEST_SCHEMA=testcpp
export TEST_USETLS=$TEST_REQUIRE_TLS

# Setting test environment before building connector to configure tests default credentials
if [ "$TRAVIS_OS_NAME" = "windows" ] ; then
  echo "build from windows"
 # set TEST_DB=testcpp
 # set TEST_TLS=%TEST_REQUIRE_TLS%
 # set TEST_UID=%TEST_DB_USER%
 # set TEST_SERVER=%TEST_DB_HOST%
 # set TEST_PASSWORD=%TEST_DB_PASSWORD%
 # set TEST_PORT=%TEST_DB_PORT%
 # set TEST_USETLS=%TEST_REQUIRE_TLS%
else
  echo "build from linux"
  export SSLCERT=$TEST_DB_SERVER_CERT
  export MARIADB_PLUGIN_DIR=$PWD

  export SSLCERT=$TEST_DB_SERVER_CERT
  if [ -n "$MYSQL_TEST_SSL_PORT" ] ; then
    export TEST_SSL_PORT=$MYSQL_TEST_SSL_PORT
  fi
fi

if [ "$TRAVIS_OS_NAME" = "windows" ] ; then
  cmake -DCONC_WITH_MSI=OFF -DCONC_WITH_UNIT_TESTS=OFF -DWITH_MSI=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_SSL=SCHANNEL -DTEST_HOST="jdbc:mariadb://$TEST_SERVER:$TEST_PORT" .
else
  if [ "$TRAVIS_OS_NAME" = "osx" ] ; then
    cmake -G Xcode -DCONC_WITH_MSI=OFF -DCONC_WITH_UNIT_TESTS=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_SSL=OPENSSL -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib -DTEST_HOST="jdbc:mariadb://$TEST_SERVER:$TEST_PORT" -DWITH_EXTERNAL_ZLIB=On .
  else
    cmake -DCONC_WITH_MSI=OFF -DCONC_WITH_UNIT_TESTS=OFF -DCMAKE_BUILD_TYPE=RelWithDebInfo -DWITH_SSL=OPENSSL -DTEST_HOST="jdbc:mariadb://$TEST_SERVER:$TEST_PORT" .
  fi
fi

set -x
cmake --build . --config RelWithDebInfo 

if [ -n "$server_branch" ] ; then

  ###################################################################################################################
  # run server test suite
  ###################################################################################################################
  echo "run server test suite"

  # change travis localhost to use only 127.0.0.1
  sudo sed -i 's/127\.0\.1\.1 localhost/127.0.0.1 localhost/' /etc/hosts
  sudo tail /etc/hosts

  # get latest server
  git clone -b ${server_branch} https://github.com/mariadb/server ../workdir-server --depth=1

  cd ../workdir-server
  export SERVER_DIR=$PWD

  # don't pull in submodules. We want the latest C/C as libmariadb
  # build latest server with latest C/C as libmariadb
  # skip to build some storage engines to speed up the build

  mkdir bld
  cd bld
  cmake .. -DPLUGIN_MROONGA=NO -DPLUGIN_ROCKSDB=NO -DPLUGIN_SPIDER=NO -DPLUGIN_TOKUDB=NO
  echo "PR:${TRAVIS_PULL_REQUEST} TRAVIS_COMMIT:${TRAVIS_COMMIT}"
  if [ -n "$TRAVIS_PULL_REQUEST" ] && [ "$TRAVIS_PULL_REQUEST" != "false" ] ; then
    # fetching pull request
    echo "fetching PR"
  else
    echo "checkout commit"
  fi

  cd $SERVER_DIR/bld
  make -j9

fi
###################################################################################################################
# run connector test suite
###################################################################################################################
echo "run connector test suite"

cd ./test
ctest --output-on-failure

