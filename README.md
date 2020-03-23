# MariaDB Connector/C++
<p align="center">
  <a href="http://mariadb.com/">
    <img src="https://mariadb.com/kb/static/images/logo-2018-black.png">
  </a>
</p>

This is a alpha release of the MariaDB Connector/C++.

MariaDB Connector/C++ is released under version 2.1 of the
GNU Lesser Public License.

License information can be found in the COPYING file.
[![License (LGPL version 2.1)](https://img.shields.io/badge/license-GNU%20LGPL%20version%202.1-green.svg?style=flat-square)](http://opensource.org/licenses/LGPL-2.1)

To report issues: [https://jira.mariadb.org/projects/CONCPP/issues/](https://jira.mariadb.org/projects/CONCPP/issues/)

## Basic Use

Connector implements JDBC API with minimal extensions and minimal
reductions of methods, that do not make sense in C++

To be able to use API, a program should include its definition in ConnCpp.h
header

\#include  <ConnCpp.h>

To obtain driver instance:
sql::Driveri\* driver= sql::mariadb::get_driver_instance();

To connect
std::unique_ptr<Connection> conn(driver->connect(url, properties));

For URL syntax and options name you may find [here](https://mariadb.com/kb/en/about-mariadb-connector-j/)
but not all options will have effect at the moment.
Properties is map of strings, and is another way to pass optional parameters.

In addition to Connector/J URL style and JDBC connect method, Connector/C++ supports
following connect methods:

std::unique_ptr<Connection> conn(driver->connect(host, username, pwd));

and

std::unique_ptr<Connection> conn(driver->connect(properties));

Host in former case is  (tcp|unix|pipe)://<host>[:port][/<db>]
Properties in the latter case are the same, as in the first variant of the connect
method, plus additionally supported
  - hostName,
  - pipe,
  - socket,
  - schema

For further use one may refer to JDBC specs.

Except Driver object, normally this is application's responsibility to delete
objects returned by the connector.
