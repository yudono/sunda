#ifndef SUNDA_MYSQL_DRIVER_H
#define SUNDA_MYSQL_DRIVER_H

#include "../db_driver.h"
#include <mysql/mysql.h>
#include <stdexcept>
#include <iostream>
#include <regex>

namespace Database {

class MySQLDriver : public DBDriver {
private:
    MYSQL* conn = nullptr;
    std::string lastError;

    struct ConnectionParams {
        std::string host = "localhost";
        std::string user = "root";
        std::string password = "";
        std::string database = "";
        int port = 3306;
    };

    ConnectionParams parseUrl(const std::string& url) {
        // mysql://user:pass@host:port/dbname
        ConnectionParams params;
        std::regex re("mysql://([^:]+):([^@]+)@([^:/]+)(?::(\\d+))?/(.+)");
        std::smatch match;
        if (std::regex_match(url, match, re)) {
            params.user = match[1].str();
            params.password = match[2].str();
            params.host = match[3].str();
            if (match[4].matched) {
                params.port = std::stoi(match[4].str());
            }
            params.database = match[5].str();
        }
        return params;
    }

public:
    MySQLDriver() {
        conn = mysql_init(nullptr);
    }

    ~MySQLDriver() {
        close();
    }

    bool connect(const std::string& url) override {
        ConnectionParams p = parseUrl(url);
        if (!mysql_real_connect(conn, p.host.c_str(), p.user.c_str(), 
                              p.password.c_str(), p.database.c_str(), 
                              p.port, nullptr, 0)) {
            lastError = mysql_error(conn);
            return false;
        }
        return true;
    }

    void close() override {
        if (conn) {
            mysql_close(conn);
            conn = nullptr;
        }
    }

    DatabaseResult query(const std::string& sql, const std::vector<Value>& params) override {
        // Parameterized queries in MySQL C API are done via mysql_stmt_prepare
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) {
            lastError = mysql_error(conn);
            return {};
        }

        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length())) {
            lastError = mysql_stmt_error(stmt);
            mysql_stmt_close(stmt);
            return {};
        }

        // Bind parameters
        std::vector<MYSQL_BIND> bind(params.size());
        memset(bind.data(), 0, sizeof(MYSQL_BIND) * params.size());

        // We need to keep strings alive until mysql_stmt_execute
        std::vector<std::string> strValues;
        for (size_t i = 0; i < params.size(); i++) {
            if (params[i].isInt) {
                bind[i].buffer_type = MYSQL_TYPE_LONG;
                bind[i].buffer = (char*)&params[i].intVal;
            } else {
                strValues.push_back(params[i].toString());
                bind[i].buffer_type = MYSQL_TYPE_STRING;
                bind[i].buffer = (char*)strValues.back().c_str();
                bind[i].buffer_length = strValues.back().length();
            }
        }

        if (mysql_stmt_bind_param(stmt, bind.data())) {
            lastError = mysql_stmt_error(stmt);
            mysql_stmt_close(stmt);
            return {};
        }

        if (mysql_stmt_execute(stmt)) {
            lastError = mysql_stmt_error(stmt);
            mysql_stmt_close(stmt);
            return {};
        }

        // Fetch results
        MYSQL_RES* prepare_meta_result = mysql_stmt_result_metadata(stmt);
        if (!prepare_meta_result) {
            mysql_stmt_close(stmt);
            return {}; // No result set
        }

        int column_count = mysql_num_fields(prepare_meta_result);
        MYSQL_FIELD* fields = mysql_fetch_fields(prepare_meta_result);

        std::vector<MYSQL_BIND> result_bind(column_count);
        memset(result_bind.data(), 0, sizeof(MYSQL_BIND) * column_count);

        struct ColumnBuffer {
            std::vector<char> buffer;
            unsigned long length;
            bool is_null;
            bool error;
        };
        std::vector<ColumnBuffer> col_buffers(column_count);

        for (int i = 0; i < column_count; i++) {
            col_buffers[i].buffer.resize(2048); // Initial buffer
            result_bind[i].buffer_type = MYSQL_TYPE_STRING;
            result_bind[i].buffer = col_buffers[i].buffer.data();
            result_bind[i].buffer_length = col_buffers[i].buffer.size();
            result_bind[i].length = &col_buffers[i].length;
            result_bind[i].is_null = &col_buffers[i].is_null;
            result_bind[i].error = &col_buffers[i].error;
        }

        if (mysql_stmt_bind_result(stmt, result_bind.data())) {
            lastError = mysql_stmt_error(stmt);
            mysql_free_result(prepare_meta_result);
            mysql_stmt_close(stmt);
            return {};
        }

        DatabaseResult results;
        while (!mysql_stmt_fetch(stmt)) {
            std::map<std::string, Value> row;
            for (int i = 0; i < column_count; i++) {
                std::string fieldName = fields[i].name;
                if (col_buffers[i].is_null) {
                    row[fieldName] = Value("", 0, false); // null as empty string? Or we need null Value
                } else {
                    std::string val(col_buffers[i].buffer.data(), col_buffers[i].length);
                    // Try to parse int if it looks like one?
                    // For now, keep as string like SQLite does unless we check field type
                    row[fieldName] = Value(val, 0, false);
                }
            }
            results.push_back(Value(row));
        }

        mysql_free_result(prepare_meta_result);
        mysql_stmt_close(stmt);
        return results;
    }

    void execute(const std::string& sql, const std::vector<Value>& params) override {
        // Same as query but no result fetching
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) {
            lastError = mysql_error(conn);
            return;
        }

        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length())) {
            lastError = mysql_stmt_error(stmt);
            mysql_stmt_close(stmt);
            return;
        }

        std::vector<MYSQL_BIND> bind(params.size());
        memset(bind.data(), 0, sizeof(MYSQL_BIND) * params.size());

        std::vector<std::string> strValues;
        for (size_t i = 0; i < params.size(); i++) {
            if (params[i].isInt) {
                bind[i].buffer_type = MYSQL_TYPE_LONG;
                bind[i].buffer = (char*)&params[i].intVal;
            } else {
                strValues.push_back(params[i].toString());
                bind[i].buffer_type = MYSQL_TYPE_STRING;
                bind[i].buffer = (char*)strValues.back().c_str();
                bind[i].buffer_length = strValues.back().length();
            }
        }

        if (mysql_stmt_bind_param(stmt, bind.data())) {
            lastError = mysql_stmt_error(stmt);
            mysql_stmt_close(stmt);
            return;
        }

        if (mysql_stmt_execute(stmt)) {
            lastError = mysql_stmt_error(stmt);
        }

        mysql_stmt_close(stmt);
    }

    std::string getLastError() override {
        return lastError;
    }
};

} // namespace Database

#endif
