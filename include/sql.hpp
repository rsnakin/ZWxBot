#pragma once

#include <sqlite3.h>
#include <string>
#include "log.hpp"
#include "main.hpp"

void initDb(sqlite3* db, Log gLog);
void insertRecord(sqlite3* db, float v1, float v2, float v3);
void trimOldRecords(sqlite3* db, int maxRows = SQLITE_RECORDS);
std::string exportToJson(sqlite3* db);

