#ifndef SUNDA_EXEC_LIB_H
#define SUNDA_EXEC_LIB_H

#include "../../core/lang/interpreter.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>

namespace ExecLib {

// exec(command) -> string (output)
Value exec_run(std::vector<Value> args) {
    if (args.empty()) return Value("", 0, false);
    std::string cmd = args[0].strVal;
    
    std::array<char, 128> buffer;
    std::string result;
    
#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
#endif

    if (!pipe) {
        return Value("Error: Failed to run command", 0, false);
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    return Value(result, 0, false);
}

void register_exec(Interpreter& interpreter) {
    interpreter.registerNative("exec_run", exec_run);
}

} // namespace ExecLib

#endif
