#include "../core/lang/interpreter.h"
#include "../core/lang/debugger.h"
#include <thread>
#include <chrono>
#include <fstream>
#include <cstdlib>
#include "gui/minigui.h" // For render_gui binding logic which might be moved here later? 
// Actually gui bindings were in anis.cpp. We should move them here OR keep them separate?
// User asked to move explicit registration to register.cpp

// Include Libs
// Include Libs
#include "math/math.h"
#include "gui/gui_lib.h"
#include "date/date_lib.h"
#include "string/string.h"
#include "array/array.h"
#include "map/map.h"
#include "database/database_lib.h"
#include "webserver/webserver_lib.h"
#include "fs/fs_lib.h"
#include "os/os_lib.h"
#include "exec/exec_lib.h"
#include "regex/regex_lib.h"
#include "json/json_lib.h"
#include "http/http_lib.h"
#include "register.h"

// Forward declare GUI registration if it's still in anis.cpp or move it?
// For now, let's implement a clean registry function.

// Header
void register_std_libs(Interpreter& interpreter) {
    // Math
    MathLib::register_math(interpreter);
    
    // GUI
    GuiLib::register_gui(interpreter);
    
    // Date
    DateLib::register_date(interpreter);
    
    // String
    register_string_lib(interpreter);
    
    // Array
    register_array_lib(interpreter);
    
    // Map
    register_map_lib(interpreter);

    // Database
    DatabaseLib::register_db(interpreter);

    // WebServer
    WebServer::register_webserver(interpreter);

    // File System
    FSLib::register_fs(interpreter);

    // OS
    OSLib::register_os(interpreter);

    // Exec
    ExecLib::register_exec(interpreter);

    // Regex
    RegexLib::register_regex(interpreter);

    // JSON
    JSONLib::register_json(interpreter);

    // HTTP
    HTTPLib::register_http(interpreter);

    // Error Class
    interpreter.registerNative("Error", [](std::vector<Value> args) -> Value {
        std::map<std::string, Value> errorObj;
        errorObj["message"] = args.empty() ? Value("", 0, false) : args[0];
        errorObj["toString"] = Value([errorObj](std::vector<Value> a) -> Value {
            return errorObj.at("message");
        });
        return Value(errorObj);
    });

    // logger Object
    std::map<std::string, Value> logger;
    logger["info"] = Value([](std::vector<Value> args) -> Value {
        for (const auto& arg : args) std::cout << arg.toString() << " ";
        std::cout << std::endl;
        return Value("", 0, false);
    });
    logger["error"] = Value([](std::vector<Value> args) -> Value {
        std::cerr << COLOR_RED;
        for (const auto& arg : args) std::cerr << arg.toString() << " ";
        std::cerr << COLOR_RESET << std::endl;
    });
    interpreter.globals->define("console", Value(logger));

    // Delay Builtin
    interpreter.registerNative("delay", [](std::vector<Value> args) -> Value {
        if (args.empty() || !args[0].isInt) return Value("", 0, false);
        std::this_thread::sleep_for(std::chrono::milliseconds(args[0].intVal));
        return Value("", 0, false);
    });

    // Env Builtin
    interpreter.registerNative("env", [](std::vector<Value> args) -> Value {
        static std::map<std::string, std::string> envCache;
        static bool loaded = false;

        if (!loaded) {
            std::ifstream file(".env");
            std::string line;
            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#') continue;
                size_t eq = line.find('=');
                if (eq != std::string::npos) {
                    std::string key = line.substr(0, eq);
                    std::string val = line.substr(eq + 1);
                    envCache[key] = val;
                }
            }
            loaded = true;
        }

        if (args.empty() || !args[0].isInt == false) return Value("", 0, false); // Expect string
        std::string key = args[0].strVal;
        
        if (envCache.count(key)) return Value(envCache[key], 0, false);
        
        const char* val = std::getenv(key.c_str());
        if (val) return Value(val, 0, false);
        
        return Value("", 0, false); 
    });

    // Number() - Convert to number
    interpreter.registerNative("Number", [](std::vector<Value> args) -> Value {
        if (args.empty()) return Value("", 0, true); // Number() = 0
        
        Value v = args[0];
        
        // Already a number
        if (v.isInt) return v;
        
        // Boolean to number
        if (v.strVal == "true") return Value("", 1, true);
        if (v.strVal == "false") return Value("", 0, true);
        
        // String to number
        if (!v.isInt) {
            try {
                int num = std::stoi(v.strVal);
                return Value("", num, true);
            } catch (...) {
                return Value("", 0, true); // NaN equivalent = 0
            }
        }
        
        return Value("", 0, true);
    });

    // String() - Convert to string
    interpreter.registerNative("String", [](std::vector<Value> args) -> Value {
        if (args.empty()) return Value("", 0, false);
        
        Value v = args[0];
        
        // Use built-in toString() method
        return Value(v.toString(), 0, false);
    });

    // Boolean() - Convert to boolean
    interpreter.registerNative("Boolean", [](std::vector<Value> args) -> Value {
        if (args.empty()) return Value("false", 0, false);
        
        Value v = args[0];
        
        // Number to boolean
        if (v.isInt) {
            return Value(v.intVal != 0 ? "true" : "false", 0, false);
        }
        
        // String to boolean (empty string = false, others = true)
        if (!v.isInt) {
            bool isTruthy = !v.strVal.empty() && v.strVal != "false" && v.strVal != "0";
            return Value(isTruthy ? "true" : "false", 0, false);
        }
        
        return Value("false", 0, false);
    });
}
