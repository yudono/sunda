#ifndef SUNDA_DATABASE_LIB_H
#define SUNDA_DATABASE_LIB_H

#include "../../core/lang/interpreter.h"
#include "db_driver.h"
#include "drivers/sqlite_driver.h"
#include "drivers/mysql_driver.h"
#include <memory>
#include <stdexcept>

namespace DatabaseLib {

class DatabaseManager {
private:
    std::unique_ptr<Database::DBDriver> driver;

public:
    bool connect(const std::string& url) {
        if (url.find("sqlite://") == 0) {
            driver = std::make_unique<Database::SQLiteDriver>();
        } else if (url.find("mysql://") == 0 || url.find("mariadb://") == 0) {
            driver = std::make_unique<Database::MySQLDriver>(); 
        } else if (url.find("postgres://") == 0 || url.find("postgresql://") == 0) {
            // driver = std::make_unique<Database::PostgresDriver>();
            throw std::runtime_error("PostgreSQL driver not yet linked. Please install libpq.");
        } else if (url.find("mongodb://") == 0) {
            throw std::runtime_error("MongoDB driver not yet implemented.");
        } else {
            throw std::runtime_error("Unsupported database scheme: " + url.substr(0, url.find("://")));
        }

        return driver->connect(url);
    }

    void close() {
        if (driver) driver->close();
    }

    Database::DatabaseResult query(const std::string& sql, const std::vector<Value>& params) {
        if (!driver) throw std::runtime_error("Database not connected.");
        return driver->query(sql, params);
    }

    void execute(const std::string& sql, const std::vector<Value>& params) {
        if (!driver) throw std::runtime_error("Database not connected.");
        driver->execute(sql, params);
    }

    std::string getError() {
        return driver ? driver->getLastError() : "No driver initialized";
    }
};

static std::shared_ptr<DatabaseManager> g_dbManager = std::make_shared<DatabaseManager>();

void register_db(Interpreter& interpreter) {
    // connect(url)
    interpreter.registerNative("db_connect", [](std::vector<Value> args) -> Value {
        if (args.empty()) return Value("", 0, false);
        try {
            bool ok = g_dbManager->connect(args[0].strVal);
            return Value("", ok ? 1 : 0, true);
        } catch (const std::exception& e) {
            std::cerr << "DB Error: " << e.what() << std::endl;
            return Value("", 0, true);
        }
    });

    // query(sql, params)
    interpreter.registerNative("db_query", [](std::vector<Value> args) -> Value {
        if (args.empty()) return Value(std::vector<Value>{});
        
        std::string sql = args[0].strVal;
        std::vector<Value> params;
        if (args.size() > 1 && args[1].isList && args[1].listVal) {
            params = *args[1].listVal;
        }

        try {
            return Value(g_dbManager->query(sql, params));
        } catch (const std::exception& e) {
            std::cerr << "DB Error: " << e.what() << std::endl;
            return Value(std::vector<Value>{});
        }
    });

    // execute(sql, params)
    interpreter.registerNative("db_execute", [](std::vector<Value> args) -> Value {
        if (args.empty()) return Value("", 0, true);
        
        std::string sql = args[0].strVal;
        std::vector<Value> params;
        if (args.size() > 1 && args[1].isList && args[1].listVal) {
            params = *args[1].listVal;
        }

        try {
            g_dbManager->execute(sql, params);
            return Value("", 1, true);
        } catch (const std::exception& e) {
            std::cerr << "DB Error: " << e.what() << std::endl;
            return Value("", 0, true);
        }
    });

    // close()
    interpreter.registerNative("db_close", [](std::vector<Value> args) -> Value {
        g_dbManager->close();
        return Value("", 1, true);
    });

    // error()
    interpreter.registerNative("db_error", [](std::vector<Value> args) -> Value {
        return Value(g_dbManager->getError(), 0, false);
    });
}

} // namespace DatabaseLib

#endif
