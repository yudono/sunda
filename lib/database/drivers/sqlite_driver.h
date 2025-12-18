#ifndef SUNDA_SQLITE_DRIVER_H
#define SUNDA_SQLITE_DRIVER_H

#include "../db_driver.h"
#include <sqlite3.h>
#include <iostream>

namespace Database {

class SQLiteDriver : public DBDriver {
private:
    sqlite3* db = nullptr;
    std::string lastError;

public:
    bool connect(const std::string& url) override {
        // url format: sqlite://path/to/db or sqlite:///absolute/path
        std::string path = url;
        if (path.find("sqlite://") == 0) {
            path = path.substr(9);
        }
        
        // std::cout << "[DB] Opening SQLite: " << path << std::endl;
        int rc = sqlite3_open(path.c_str(), &db);
        if (rc != SQLITE_OK) {
            lastError = db ? sqlite3_errmsg(db) : "Failed to allocate sqlite3 handle";
            if (db) {
                sqlite3_close(db);
                db = nullptr;
            }
            return false;
        }
        return true;
    }

    void close() override {
        if (db) {
            sqlite3_close(db);
            db = nullptr;
        }
    }

    void execute(const std::string& sql, const std::vector<Value>& params) override {
        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        
        if (rc != SQLITE_OK) {
            lastError = sqlite3_errmsg(db);
            return;
        }

        bindParams(stmt, params);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            lastError = sqlite3_errmsg(db);
        }

        sqlite3_finalize(stmt);
    }

    DatabaseResult query(const std::string& sql, const std::vector<Value>& params) override {
        DatabaseResult result;
        sqlite3_stmt* stmt;
        
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            lastError = sqlite3_errmsg(db);
            return result;
        }

        bindParams(stmt, params);

        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            std::map<std::string, Value> row;
            int count = sqlite3_column_count(stmt);
            
            for (int i = 0; i < count; i++) {
                std::string name = sqlite3_column_name(stmt, i);
                int type = sqlite3_column_type(stmt, i);
                
                if (type == SQLITE_INTEGER) {
                    row[name] = Value("", sqlite3_column_int(stmt, i), true);
                } else if (type == SQLITE_NULL) {
                    row[name] = Value("null", 0, false);
                } else {
                    const char* text = (const char*)sqlite3_column_text(stmt, i);
                    row[name] = Value(text ? text : "", 0, false);
                }
            }
            result.push_back(Value(row));
        }

        if (rc != SQLITE_DONE) {
            lastError = sqlite3_errmsg(db);
        }

        sqlite3_finalize(stmt);
        return result;
    }

    std::string getLastError() override {
        return lastError;
    }

private:
    void bindParams(sqlite3_stmt* stmt, const std::vector<Value>& params) {
        for (size_t i = 0; i < params.size(); i++) {
            const Value& v = params[i];
            int idx = i + 1;
            if (v.isInt) {
                sqlite3_bind_int(stmt, idx, v.intVal);
            } else {
                sqlite3_bind_text(stmt, idx, v.strVal.c_str(), -1, SQLITE_TRANSIENT);
            }
        }
    }
};

} // namespace Database

#endif
