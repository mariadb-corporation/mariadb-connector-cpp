# Benchmark MariaDB Connector/C++

This permits to benchmark MariaDB connector/C++, comparing with MySQL connector

## Installation
To install google benchmark, mysql connector and build current connector :
```script
  sudo benchmark/build.sh
  cd benchmark
  sudo ./installation.sh
```

## Basic run

This will runs the benchmark with 50 repetition to ensure stability then display results
```script
sudo ./launch.sh
```

## detailed benchmark

first ensure running cpu to maximum speed:
```script
sudo cpupower frequency-set --governor performance || true
```

launch a server:
```script
sudo docker run --name mariadb-bench -e MARIADB_DATABASE=bench -e MARIADB_USER=example-user -e MARIADB_PASSWORD=my_cool_secret -e MARIADB_ROOT_PASSWORD=my-secret-pw -p 3806:3306 -d mariadb:latest --max_connections=10000 --character-set-server=utf8mb4 --collation-server=utf8mb4_unicode_ci --innodb_flush_log_at_trx_commit=2 --innodb_doublewrite=0 --innodb_log_file_size=10G --innodb_buffer_pool_size=10G --thread_handling=pool-of-threads --max_heap_table_size=6G --tmp_table_size=6G --innodb_io_capacity=30000
```

default is benchmarking on one thread. adding benchmark on multiple thread can be done setting MAX_THREAD in main-benchmark.cc. Setting it to 256, benchmark will run on 1 to 256 threads. 

running with MariaDB driver:
```script
g++ main-benchmark.cc -std=c++11 -isystem benchmark/include -Lbenchmark/build/src -L/usr/local/lib/mariadb/ -lbenchmark -lpthread -lmariadbcpp -o main-benchmark
./main-benchmark --benchmark_repetitions=50 --benchmark_time_unit=us --benchmark_min_warmup_time=10 --benchmark_counters_tabular=true --benchmark_format=json --benchmark_out=mariadb.json
```

running with MySQL driver:
```script
g++ main-benchmark.cc -std=c++11 -isystem benchmark/include -Lbenchmark/build/src -lbenchmark -lpthread -DMYSQL -lmysqlcppconn -o main-benchmark
./main-benchmark --benchmark_repetitions=50 --benchmark_time_unit=us --benchmark_min_warmup_time=10 --benchmark_counters_tabular=true --benchmark_format=json --benchmark_out=mysql.json
```

in order to compare results:

```script
pip3 install -r benchmark/requirements.txt
benchmark/tools/compare.py -a --no-utest benchmarksfiltered ./mysql.json MySQL ./mariadb.json MariaDB
```
