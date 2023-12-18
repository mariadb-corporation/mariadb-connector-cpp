#include <benchmark/benchmark.h>
#include <iostream>
#include <string>
#include <cstring>
#include <stdlib.h>

const int MAX_THREAD = 1;
#define OPERATION_PER_SECOND_LABEL "nb operations per second"

std::string GetEnvironmentVariableOrDefault(const std::string& variable_name,
                                            const std::string& default_value)
{
    const char* value = getenv(variable_name.c_str());
    return value ? value : default_value;
}

std::string DB_PORT = GetEnvironmentVariableOrDefault("TEST_DB_PORT", "3306");
std::string DB_DATABASE = GetEnvironmentVariableOrDefault("TEST_DB_DATABASE", "bench");
std::string DB_USER = GetEnvironmentVariableOrDefault("TEST_DB_USER", "root");
std::string DB_HOST = GetEnvironmentVariableOrDefault("TEST_DB_HOST", "localhost");
std::string DB_PASSWORD = GetEnvironmentVariableOrDefault("TEST_DB_PASSWORD", "");

#ifndef MYSQL
    #include <mariadb/conncpp.hpp>
const std::string TYPE = "MariaDB";
sql::Connection* connect(std::string options) {
    try {
        sql::Connection *con;
        sql::Driver *driver;

        // Instantiate Driver
        driver = sql::mariadb::get_driver_instance();

         // Establish Connection
         con = driver->connect("tcp://" + DB_HOST + ":" + DB_PORT + "/" + DB_DATABASE + options, DB_USER, DB_PASSWORD);
         return con;

     } catch(sql::SQLException& e){
         std::cerr << "Error Connecting to MariaDB Platform: " << e.what() << std::endl;
         // Exit (Failed)
         exit(1) ;
     }
}
#endif

#ifdef MYSQL
    #include "mysql_connection.h"
    #include <cppconn/driver.h>
    #include <cppconn/exception.h>
    #include <cppconn/resultset.h>
    #include <cppconn/statement.h>
    #include <cppconn/prepared_statement.h>
const std::string TYPE = "MySQL";
sql::Connection* connect(std::string options) {
    try {
        sql::Connection *con;
        sql::Driver *driver;

        // Instantiate Driver
        driver = get_driver_instance();
         //sql::mariadb::get_driver_instance();

        // Configure Connection

         // Establish Connection
         con = driver->connect("tcp://" + DB_HOST + ":" + DB_PORT + "/" + DB_DATABASE + options, DB_USER, DB_PASSWORD);
         return con;

     } catch(sql::SQLException& e){
         std::cerr << "Error Connecting to MySQL Platform: " << e.what() << std::endl;
         // Exit (Failed)
         exit(1) ;
     }
}
#endif

void do_1(benchmark::State& state, sql::Connection* conn) {
  try {
    sql::Statement *stmt;
    sql::ResultSet *res;

    // Create a new Statement
    stmt = conn->createStatement();
    // Execute query
    stmt->executeUpdate("DO 1");
    delete stmt;
  } catch(sql::SQLException& e){
    state.SkipWithError(e.what());
  }
}

static void BM_DO_1(benchmark::State& state) {
  sql::Connection *conn = connect("");
  int numOperation = 0;
  for (auto _ : state) {
    do_1(state, conn);
    numOperation++;
  }
  state.counters[OPERATION_PER_SECOND_LABEL] = benchmark::Counter(numOperation, benchmark::Counter::kIsRate);
  delete conn;
}

BENCHMARK(BM_DO_1)->Name(TYPE + " DO 1")->ThreadRange(1, MAX_THREAD)->UseRealTime();

int select_1(benchmark::State& state, sql::Connection* conn) {
    int val;
    try {
        sql::Statement *stmt;
        sql::ResultSet *res;

        // Create a new Statement
        stmt = conn->createStatement();
        // Execute query
        res = stmt->executeQuery("SELECT 1");
        // Loop through results
        while (res->next()) {
          val = res->getInt(1);
        }
        delete res;
        delete stmt;
    } catch(sql::SQLException& e){
        state.SkipWithError(e.what());
    }
    return val;
}

static void BM_SELECT_1(benchmark::State& state) {
  sql::Connection *conn = connect("");
  int numOperation = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(select_1(state, conn));
    benchmark::ClobberMemory();
    numOperation++;
  }
  state.counters[OPERATION_PER_SECOND_LABEL] = benchmark::Counter(numOperation, benchmark::Counter::kIsRate);
  delete conn;
}

BENCHMARK(BM_SELECT_1)->Name(TYPE + " SELECT 1")->ThreadRange(1, MAX_THREAD)->UseRealTime();

void select_1000_rows(benchmark::State& state, sql::Connection* conn) {
  try {
    sql::Statement *stmt;
    sql::ResultSet *res;

    // Create a new Statement
    stmt = conn->createStatement();
    // stmt->setFetchSize(1); => error if set
    // Execute query
    res = stmt->executeQuery("select seq, 'abcdefghijabcdefghijabcdefghijaa' from seq_1_to_1000");

    // Loop through results
    int val1;
    sql::SQLString val2;
    while (res->next()) {
        benchmark::DoNotOptimize(val1 = res->getInt(1));
        benchmark::DoNotOptimize(val2 = res->getString(2));
        benchmark::ClobberMemory();
    }
    delete res;
    delete stmt;
  } catch(sql::SQLException& e){
      state.SkipWithError(e.what());
  }
}

static void BM_SELECT_1000_ROWS(benchmark::State& state) {
  sql::Connection *conn = connect("");
  int numOperation = 0;
  for (auto _ : state) {
    select_1000_rows(state, conn);
    numOperation++;
  }
  state.counters[OPERATION_PER_SECOND_LABEL] = benchmark::Counter(numOperation, benchmark::Counter::kIsRate);
  delete conn;
}

BENCHMARK(BM_SELECT_1000_ROWS)->Name(TYPE + " SELECT 1000 rows (int + char(32))")->ThreadRange(1, MAX_THREAD)->UseRealTime();

static void setup_select_100_cols(const benchmark::State& state) {
  sql::Connection *conn = connect("");

  // initialize data
  sql::Statement *stmt;
  stmt = conn->createStatement();
  stmt->executeUpdate("DROP TABLE IF EXISTS test100");
  stmt->execute("CREATE TABLE test100 (i1 int,i2 int,i3 int,i4 int,i5 int,i6 int,i7 int,i8 int,i9 int,i10 int,i11 int,i12 int,i13 int,i14 int,i15 int,i16 int,i17 int,i18 int,i19 int,i20 int,i21 int,i22 int,i23 int,i24 int,i25 int,i26 int,i27 int,i28 int,i29 int,i30 int,i31 int,i32 int,i33 int,i34 int,i35 int,i36 int,i37 int,i38 int,i39 int,i40 int,i41 int,i42 int,i43 int,i44 int,i45 int,i46 int,i47 int,i48 int,i49 int,i50 int,i51 int,i52 int,i53 int,i54 int,i55 int,i56 int,i57 int,i58 int,i59 int,i60 int,i61 int,i62 int,i63 int,i64 int,i65 int,i66 int,i67 int,i68 int,i69 int,i70 int,i71 int,i72 int,i73 int,i74 int,i75 int,i76 int,i77 int,i78 int,i79 int,i80 int,i81 int,i82 int,i83 int,i84 int,i85 int,i86 int,i87 int,i88 int,i89 int,i90 int,i91 int,i92 int,i93 int,i94 int,i95 int,i96 int,i97 int,i98 int,i99 int,i100 int)");
  stmt->execute("INSERT INTO test100 value (1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100)");
  delete stmt;
  delete conn;
}


void select_100_cols(benchmark::State& state, sql::Connection* conn) {
    try {
        sql::PreparedStatement *prep_stmt;
        sql::ResultSet *res;

        // Create a new Statement
        prep_stmt = conn->prepareStatement("select * FROM test100");
        // Execute query
        res = prep_stmt->executeQuery();
        // Loop through results
        int val;
        while (res->next()) {
            for (int i=1; i<=100; i++)
               benchmark::DoNotOptimize(val = res->getInt(i));
               benchmark::ClobberMemory();
        }
        delete res;
        delete prep_stmt;
    } catch(sql::SQLException& e){
        state.SkipWithError(e.what());
    }
}

static void BM_SELECT_100_COLS(benchmark::State& state) {
  sql::Connection *conn = connect("");
  int numOperation = 0;
  for (auto _ : state) {
    select_100_cols(state, conn);
    numOperation++;
  }
  state.counters[OPERATION_PER_SECOND_LABEL] = benchmark::Counter(numOperation, benchmark::Counter::kIsRate);
  delete conn;
}
static void BM_SELECT_100_COLS_SRV_PREPARED(benchmark::State& state) {
  sql::Connection *conn = connect("?useServerPrepStmts=true");
  int numOperation = 0;
  for (auto _ : state) {
    select_100_cols(state, conn);
    numOperation++;
  }
  state.counters[OPERATION_PER_SECOND_LABEL] = benchmark::Counter(numOperation, benchmark::Counter::kIsRate);
  delete conn;
}

BENCHMARK(BM_SELECT_100_COLS)->Name(TYPE + " SELECT 100 int cols")->ThreadRange(1, MAX_THREAD)->UseRealTime()->Setup(setup_select_100_cols);
#ifndef MYSQL
BENCHMARK(BM_SELECT_100_COLS_SRV_PREPARED)->Name(TYPE + " SELECT 100 int cols - srv prepared")->ThreadRange(1, MAX_THREAD)->UseRealTime();
#endif


void do_1000_params(benchmark::State& state, sql::Connection* conn) {
     try {
         sql::PreparedStatement *prep_stmt;
         sql::ResultSet *res;

         // Create a new Statement
         prep_stmt = conn->prepareStatement(
            "DO ?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?"
         );
         for (int i=1; i<=1000; i++) {
            prep_stmt->setInt(i, i);
         }

         // Execute query
         prep_stmt->execute();
         delete prep_stmt;
    } catch(sql::SQLException& e){
        state.SkipWithError(e.what());
    }
}

static void BM_DO_1000_PARAMS(benchmark::State& state) {
  sql::Connection *conn = connect("");
  int numOperation = 0;
  for (auto _ : state) {
    do_1000_params(state, conn);
    numOperation++;
  }
  state.counters[OPERATION_PER_SECOND_LABEL] = benchmark::Counter(numOperation, benchmark::Counter::kIsRate);
  delete conn;
}

BENCHMARK(BM_DO_1000_PARAMS)->Name(TYPE + " DO 1000 params")->ThreadRange(1, MAX_THREAD)->UseRealTime();

#ifndef MYSQL

    std::vector<std::string> chars = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "\\Z", "😎", "🌶", "🎤", "🥂" };

    std::string randomString(int length) {
        std::string result = "";
        for (int i = length; i > 0; --i) {
            result += chars[rand() % (chars.size() - 1)];
        }
        return result;
    }
    static void setup_insert_batch(const benchmark::State& state) {
      sql::Connection *conn = connect("");

      sql::Statement *stmt;
      stmt = conn->createStatement();
      stmt->executeUpdate("DROP TABLE IF EXISTS perfTestTextBatch");
      stmt->executeUpdate("INSTALL SONAME 'ha_blackhole'");
      try {
        stmt->executeUpdate("CREATE TABLE perfTestTextBatch (id MEDIUMINT NOT NULL AUTO_INCREMENT,t0 text, PRIMARY KEY (id)) COLLATE='utf8mb4_unicode_ci' ENGINE = BLACKHOLE");
      } catch(sql::SQLException& e){
        stmt->executeUpdate("CREATE TABLE perfTestTextBatch (id MEDIUMINT NOT NULL AUTO_INCREMENT,t0 text, PRIMARY KEY (id)) COLLATE='utf8mb4_unicode_ci'");
      }
      delete stmt;
      delete conn;
    }

    void insert_batch_with_prepare(benchmark::State& state, sql::Connection* conn) {
      std::string query = "INSERT INTO perfTestTextBatch(t0) VALUES (?)";
      int rc;
      std::string randomStringVal = randomString(100);

      try {
        sql::PreparedStatement *prep_stmt;
        sql::ResultSet *res;

        // Create a new Statement
        prep_stmt = conn->prepareStatement(query);
        for (int i = 0; i < 100; i++) {
          prep_stmt->setString(1, randomStringVal);
          prep_stmt->addBatch();
        }

        // Execute query
        prep_stmt->executeBatch();
        delete prep_stmt;
      } catch(sql::SQLException& e){
        state.SkipWithError(e.what());
      }
    }

    static void BM_INSERT_BATCH_CLIENT_REWRITE(benchmark::State& state) {
      sql::Connection *conn = connect("?rewriteBatchedStatements=true");
      int numOperation = 0;
      for (auto _ : state) {
        insert_batch_with_prepare(state, conn);
        numOperation++;
      }
      state.counters[OPERATION_PER_SECOND_LABEL] = benchmark::Counter(numOperation, benchmark::Counter::kIsRate);
      delete conn;
    }

    static void BM_INSERT_BATCH_WITH_PREPARE(benchmark::State& state) {
      sql::Connection *conn = connect("?useServerPrepStmts=true");
      int numOperation = 0;
      for (auto _ : state) {
        insert_batch_with_prepare(state, conn);
        numOperation++;
      }
      state.counters[OPERATION_PER_SECOND_LABEL] = benchmark::Counter(numOperation, benchmark::Counter::kIsRate);
      delete conn;
    }
    BENCHMARK(BM_INSERT_BATCH_WITH_PREPARE)->Name(TYPE + " insert batch looping execute")->ThreadRange(1, MAX_THREAD)->UseRealTime()->Setup(setup_insert_batch);
    BENCHMARK(BM_INSERT_BATCH_CLIENT_REWRITE)->Name(TYPE + " insert batch client rewrite")->ThreadRange(1, MAX_THREAD)->UseRealTime()->Setup(setup_insert_batch);
#endif

BENCHMARK_MAIN();

