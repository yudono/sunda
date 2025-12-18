#ifndef SUNDA_DB_DRIVER_H
#define SUNDA_DB_DRIVER_H

#include "../../core/lang/interpreter.h"
#include <vector>
#include <map>
#include <string>
#include <memory>

namespace Database {

// Result set is a list of maps (dictionary rows)
typedef std::vector<Value> DatabaseResult;

class DBDriver {
public:
    virtual ~DBDriver() {}
    
    // Connection
    virtual bool connect(const std::string& url) = 0;
    virtual void close() = 0;
    
    // Querying (returns results)
    virtual DatabaseResult query(const std::string& sql, const std::vector<Value>& params) = 0;
    
    // Execution (inserts, updates, deletes - no results)
    virtual void execute(const std::string& sql, const std::vector<Value>& params) = 0;
    
    // Error handling
    virtual std::string getLastError() = 0;
};

} // namespace Database

#endif
