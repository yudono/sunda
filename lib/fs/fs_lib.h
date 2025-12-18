#ifndef SUNDA_FS_LIB_H
#define SUNDA_FS_LIB_H

#include "../../core/lang/interpreter.h"
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace FSLib {

// readFile(path) -> string
Value fs_readFile(std::vector<Value> args) {
    if (args.empty()) return Value("", 0, false);
    std::string path = args[0].strVal;
    
    std::ifstream file(path);
    if (!file.is_open()) return Value("undefined", 0, false);
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return Value(buffer.str(), 0, false);
}

// writeFile(path, content) -> bool
Value fs_writeFile(std::vector<Value> args) {
    if (args.size() < 2) return Value("", 0, true);
    std::string path = args[0].strVal;
    std::string content = args[1].toString();
    
    std::ofstream file(path);
    if (!file.is_open()) return Value("", 0, true);
    
    file << content;
    return Value("", 1, true);
}

// exists(path) -> bool
Value fs_exists(std::vector<Value> args) {
    if (args.empty()) return Value("", 0, true);
    return Value("", fs::exists(args[0].strVal) ? 1 : 0, true);
}

// isDirectory(path) -> bool
Value fs_isDirectory(std::vector<Value> args) {
    if (args.empty()) return Value("", 0, true);
    return Value("", fs::is_directory(args[0].strVal) ? 1 : 0, true);
}

// listDir(path) -> array of strings
Value fs_listDir(std::vector<Value> args) {
    std::string path = args.empty() ? "." : args[0].strVal;
    std::vector<Value> result;
    
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            result.push_back(Value(entry.path().filename().string(), 0, false));
        }
    } catch (...) {
        return Value(std::vector<Value>{});
    }
    
    return Value(result);
}

// mkdir(path) -> bool
Value fs_mkdir(std::vector<Value> args) {
    if (args.empty()) return Value("", 0, true);
    try {
        bool ok = fs::create_directories(args[0].strVal);
        return Value("", ok ? 1 : 0, true);
    } catch (...) {
        return Value("", 0, true);
    }
}

// remove(path) -> bool
Value fs_remove(std::vector<Value> args) {
    if (args.empty()) return Value("", 0, true);
    try {
        bool ok = fs::remove_all(args[0].strVal) > 0;
        return Value("", ok ? 1 : 0, true);
    } catch (...) {
        return Value("", 0, true);
    }
}

void register_fs(Interpreter& interpreter) {
    interpreter.registerNative("fs_readFile", fs_readFile);
    interpreter.registerNative("fs_writeFile", fs_writeFile);
    interpreter.registerNative("fs_exists", fs_exists);
    interpreter.registerNative("fs_isDirectory", fs_isDirectory);
    interpreter.registerNative("fs_listDir", fs_listDir);
    interpreter.registerNative("fs_mkdir", fs_mkdir);
    interpreter.registerNative("fs_remove", fs_remove);
}

} // namespace FSLib

#endif
