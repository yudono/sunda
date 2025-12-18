#ifndef DATE_LIB_H
#define DATE_LIB_H

#include "core/lang/interpreter.h"
#include <chrono>

struct DateLib {
    static void register_date(Interpreter& interpreter) {
        // DateNow() - returns milliseconds since epoch
        auto dateNow = [](std::vector<Value> args) {
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            return Value("", static_cast<int>(millis), true);
        };
        interpreter.globals->define("DateNow", Value(dateNow));
        interpreter.natives["DateNow"] = dateNow;
    }
};

#endif
