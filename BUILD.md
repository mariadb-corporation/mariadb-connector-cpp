# Building the Connector/C++

Following are build instructions for various operating systems. In order to build you will need compiler
supporting C++11 standard.

## Windows

Prior to start building on Windows you need to have following tools installed:
- Microsoft Visual Studio https://visualstudio.microsoft.com/downloads/
- Git https://git-scm.com/download/win
- cmake https://cmake.org/download/

```
git clone https://github.com/MariaDB/mariadb-connector-odbc.git
cd mariadb-connector-odbc
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCONC_WITH_UNIT_TESTS=Off -DCONC_WITH_MSI=OFF -DWITH_SSL=SCHANNEL .
cmake --build . --config RelWithDebInfo
msiexec.exe /i wininstall\mariadb-connector-cpp-0.9.1-win32.msi
```

## CentOS

```
sudo yum -y install git cmake make gcc openssl-devel unixODBC unixODBC-devel
git clone https://github.com/MariaDB/mariadb-connector-odbc.git
mkdir build && cd build
cmake ../mariadb-connector-odbc/ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCONC_WITH_UNIT_TESTS=Off -DCMAKE_INSTALL_PREFIX=/usr/local -DWITH_SSL=OPENSSL
cmake --build . --config RelWithDebInfo
sudo make install
```

## Debian & Ubuntu

```
sudo apt-get update
sudo sh apt-get install -y git cmake make gcc libssl-dev unixodbc-dev
git clone https://github.com/MariaDB/mariadb-connector-odbc.git
mkdir build && cd build
cmake ../mariadb-connector-odbc/ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCONC_WITH_UNIT_TESTS=Off -DCMAKE_INSTALL_PREFIX=/usr/local -DWITH_SSL=OPENSSL
cmake --build . --config RelWithDebInfo
sudo make install
```
