#ifndef SUNDA_OS_LIB_H
#define SUNDA_OS_LIB_H

#include "../../core/lang/interpreter.h"
#include <cstdlib>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#define getcwd _getcwd
#else
#include <unistd.h>
#endif

namespace OSLib {

// getenv(name) -> string
Value os_getenv(std::vector<Value> args) {
    if (args.empty()) return Value("undefined", 0, false);
    char* val = std::getenv(args[0].strVal.c_str());
    if (!val) return Value("undefined", 0, false);
    return Value(std::string(val), 0, false);
}

// setenv(name, value) -> bool
Value os_setenv(std::vector<Value> args) {
    if (args.size() < 2) return Value("", 0, true);
#ifdef _WIN32
    int res = _putenv_s(args[0].strVal.c_str(), args[1].toString().c_str());
#else
    int res = setenv(args[0].strVal.c_str(), args[1].toString().c_str(), 1);
#endif
    return Value("", res == 0 ? 1 : 0, true);
}

// platform() -> string
Value os_platform(std::vector<Value> args) {
#ifdef _WIN32
    return Value("windows", 0, false);
#elif __APPLE__
    return Value("macos", 0, false);
#else
    return Value("linux", 0, false);
#endif
}

// cwd() -> string
Value os_cwd(std::vector<Value> args) {
    char buf[1024];
    if (getcwd(buf, sizeof(buf))) {
        return Value(std::string(buf), 0, false);
    }
    return Value("", 0, false);
}

void register_os(Interpreter& interpreter) {
    interpreter.registerNative("os_getenv", os_getenv);
    interpreter.registerNative("os_setenv", os_setenv);
    interpreter.registerNative("os_platform", os_platform);
    interpreter.registerNative("os_cwd", os_cwd);
}

} // namespace OSLib

#endif
