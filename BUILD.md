# Building the Connector/C++

Following are build instructions for various operating systems. In order to build you will need compiler
supporting C++11 standard.

## Windows

Prior to start building on Windows you need to have following tools installed:
- Microsoft Visual Studio https://visualstudio.microsoft.com/downloads/
- Git https://git-scm.com/download/win
- cmake https://cmake.org/download/
- WiX Toolset https://wixtoolset.org/releases/

```
git clone https://github.com/MariaDB-Corporation/mariadb-connector-cpp.git
cd mariadb-connector-cpp
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCONC_WITH_UNIT_TESTS=Off -DCONC_WITH_MSI=OFF -DWITH_SSL=SCHANNEL .
cmake --build . --config RelWithDebInfo
msiexec.exe /i wininstall\mariadb-connector-cpp-0.9.1-win32.msi
```
Please mind msi file name - it's will be different depending on current connector version, and also architecture suffix may be different
 
## CentOS

```
sudo yum -y install git cmake make gcc openssl-devel
git clone https://github.com/MariaDB-Corporation/mariadb-connector-cpp.git
mkdir build && cd build
cmake ../mariadb-connector-cpp/ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCONC_WITH_UNIT_TESTS=Off -DCMAKE_INSTALL_PREFIX=/usr/local -DWITH_SSL=OPENSSL
cmake --build . --config RelWithDebInfo
sudo make install
```

## Debian & Ubuntu

```
sudo apt-get update
sudo sh apt-get install -y git cmake make gcc libssl-dev
git clone https://github.com/MariaDB-Corporation/mariadb-connector-cpp.git
mkdir build && cd build
cmake ../mariadb-connector-cpp/ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCONC_WITH_UNIT_TESTS=Off -DCMAKE_INSTALL_PREFIX=/usr/local -DWITH_SSL=OPENSSL
cmake --build . --config RelWithDebInfo
sudo make install
```
