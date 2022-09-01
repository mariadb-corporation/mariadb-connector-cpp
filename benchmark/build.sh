#!/bin/bash

set -ex

sudo apt-get update
sudo apt-get -f -y install build-essential cmake
sudo apt-get -f -y install libmariadb3 libmariadb-dev
sudo apt-get -f -y install linux-tools-common linux-gcp linux-tools-$(uname -r)
cmake . -DCONC_WITH_MSI=OFF -DCONC_WITH_UNIT_TESTS=OFF -DCMAKE_BUILD_TYPE=Release -DWITH_SSL=OPENSSL
sudo cmake --build . --config Release  --target install
sudo make install
echo $LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/lib
sudo install /usr/local/lib/mariadb/libmariadbcpp.so /usr/lib
sudo install -d /usr/lib/mariadb
sudo install -d /usr/lib/mariadb/plugin

sudo ldconfig -n /usr/local/lib/mariadb || true
sudo ldconfig -l -v /usr/lib/libmariadbcpp.so || true

ls -lrt /usr/lib/libmar*

#ldconfig -p

#install mysql
#git clone --recurse-submodules https://github.com/mysql/mysql-connector-cpp.git
#cd mysql-connector-cpp
#git checkout 8.0
#
#sudo apt-get install libboost-all-dev
#
#cmake . -DCMAKE_BUILD_TYPE=Release -DWITH_JDBC=ON -DWITH_BOOST=/usr/include/boost
#sudo cmake --build . --target install --config Release


sudo apt-get -y install libmysqlcppconn-dev
