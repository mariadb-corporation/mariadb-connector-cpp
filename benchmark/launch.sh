#!/bin/bash

set -ex

sudo cpupower frequency-set --governor performance || true

g++ main-benchmark.cc -std=c++11 -isystem benchmark/include -Lbenchmark/build/src -L/usr/local/lib/mariadb/ -lbenchmark -lpthread -lmariadbcpp -o main-benchmark
./main-benchmark --benchmark_repetitions=30 --benchmark_time_unit=us --benchmark_min_warmup_time=10 --benchmark_counters_tabular=true --benchmark_format=json --benchmark_out=mariadb.json

g++ main-benchmark.cc -std=c++11 -isystem benchmark/include -Lbenchmark/build/src -lbenchmark -lpthread -DMYSQL -lmysqlcppconn -o main-benchmark
./main-benchmark --benchmark_repetitions=30 --benchmark_time_unit=us --benchmark_min_warmup_time=10 --benchmark_counters_tabular=true --benchmark_format=json --benchmark_out=mysql.json


pip3 install -r benchmark/requirements.txt
benchmark/tools/compare.py -a --no-utest benchmarksfiltered ./mysql.json MySQL ./mariadb.json MariaDB