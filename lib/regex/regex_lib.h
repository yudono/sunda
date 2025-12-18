#ifndef SUNDA_REGEX_LIB_H
#define SUNDA_REGEX_LIB_H

#include "../../core/lang/interpreter.h"
#include <regex>
#include <string>
#include <vector>

namespace RegexLib {

// match(str, pattern) -> bool
Value regex_match(std::vector<Value> args) {
    if (args.size() < 2) return Value("", 0, true);
    std::string str = args[0].toString();
    std::string pattern = args[1].toString();
    try {
        std::regex re(pattern, std::regex_constants::ECMAScript);
        return Value("", std::regex_match(str, re) ? 1 : 0, true);
    } catch (const std::regex_error& e) {
        std::cerr << "Regex error: " << e.what() << " in pattern: " << pattern << std::endl;
        return Value("", 0, true);
    }
}

// search(str, pattern) -> bool
Value regex_search(std::vector<Value> args) {
    if (args.size() < 2) return Value("", 0, true);
    std::string str = args[0].toString();
    std::string pattern = args[1].toString();
    try {
        std::regex re(pattern, std::regex_constants::ECMAScript);
        return Value("", std::regex_search(str, re) ? 1 : 0, true);
    } catch (const std::regex_error& e) {
        std::cerr << "Regex error: " << e.what() << " in pattern: " << pattern << std::endl;
        return Value("", 0, true);
    }
}

// replace(str, pattern, replacement) -> string
Value regex_replace(std::vector<Value> args) {
    if (args.size() < 3) return Value("", 0, false);
    std::string str = args[0].toString();
    std::string pattern = args[1].toString();
    std::string replacement = args[2].toString();
    try {
        std::regex re(pattern, std::regex_constants::ECMAScript);
        return Value(std::regex_replace(str, re, replacement), 0, false);
    } catch (const std::regex_error& e) {
        std::cerr << "Regex error: " << e.what() << " in pattern: " << pattern << std::endl;
        return Value(str, 0, false);
    }
}

void register_regex(Interpreter& interpreter) {
    interpreter.registerNative("regex_match", regex_match);
    interpreter.registerNative("regex_search", regex_search);
    interpreter.registerNative("regex_replace", regex_replace);
}

} // namespace RegexLib

#endif
