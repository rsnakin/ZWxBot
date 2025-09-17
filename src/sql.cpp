#include <sqlite3.h>
#include <string>
#include <sstream>
#include <iostream>
#include <ctime>
#include "log.hpp"
#include "sql.hpp"

void initDb(sqlite3* db, Log gLog) {
    const char* sql = "CREATE TABLE IF NOT EXISTS records ("
                     "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                     "timestamp TEXT NOT NULL,"
                     "value1 REAL,"
                     "value2 REAL,"
                     "value3 REAL);";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Error creating table: " << errMsg << std::endl;
        gLog.write("Error creating table 'records' %s", errMsg);
        sqlite3_free(errMsg);
    }
}
void insertRecord(sqlite3* db, float v1, float v2, float v3) {
    std::time_t now = std::time(nullptr);
    std::string timestamp = std::string(std::ctime(&now));
    timestamp.pop_back();

    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO records (timestamp, value1, value2, value3) VALUES (?, ?, ?, ?)";
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, timestamp.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, v1);
    sqlite3_bind_double(stmt, 3, v2);
    sqlite3_bind_double(stmt, 4, v3);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}
void trimOldRecords(sqlite3* db, int maxRows) {
    const char* sql = "DELETE FROM records WHERE id NOT IN ("
                     "SELECT id FROM records ORDER BY id DESC LIMIT ?);";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, maxRows);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}
std::string exportToJson(sqlite3* db) {
    std::ostringstream json;
    json << "[";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT timestamp, value1, value2, value3 FROM records ORDER BY id", -1, &stmt, nullptr);

    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!first) json << ",";
        first = false;
        json << "{";
        json << "\"t0\":\"" << sqlite3_column_text(stmt, 0) << "\",";
        json << "\"v1\":" << sqlite3_column_double(stmt, 1) << ",";
        json << "\"v2\":" << sqlite3_column_double(stmt, 2) << ",";
        json << "\"v3\":" << sqlite3_column_double(stmt, 3);
        json << "}";
    }

    sqlite3_finalize(stmt);
    json << "]";
    return json.str();
}
