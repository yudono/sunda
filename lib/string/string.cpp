#include "string.h"
#include <algorithm>
#include <cctype>
#include <sstream>

// Helper: trim whitespace
std::string trim_helper(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

// split(str, delimiter) -> array
Value string_split(std::vector<Value> args) {
    if (args.empty()) return Value(std::vector<Value>{});
    
    std::string str = args[0].toString();
    std::string delimiter = (args.size() > 1) ? args[1].toString() : ",";
    
    std::vector<Value> result;
    if (delimiter.empty()) {
        // Split into characters
        for (char c : str) {
            result.push_back(Value(std::string(1, c), 0, false));
        }
        return Value(result);
    }
    
    size_t start = 0;
    size_t end = str.find(delimiter);
    
    while (end != std::string::npos) {
        result.push_back(Value(str.substr(start, end - start), 0, false));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    result.push_back(Value(str.substr(start), 0, false));
    
    return Value(result);
}

// join(array, separator) -> string
Value string_join(std::vector<Value> args) {
    if (args.empty() || !args[0].isList) return Value("", 0, false);
    
    std::string separator = (args.size() > 1) ? args[1].toString() : ",";
    std::string result;
    
    auto& list = *args[0].listVal;
    for (size_t i = 0; i < list.size(); i++) {
        result += list[i].toString();
        if (i < list.size() - 1) result += separator;
    }
    
    return Value(result, 0, false);
}

// trim(str) -> string
Value string_trim(std::vector<Value> args) {
    if (args.empty()) return Value("", 0, false);
    return Value(trim_helper(args[0].toString()), 0, false);
}

// replace(str, search, replace) -> string
Value string_replace(std::vector<Value> args) {
    if (args.size() < 3) return Value("", 0, false);
    
    std::string str = args[0].toString();
    std::string search = args[1].toString();
    std::string replace = args[2].toString();
    
    if (search.empty()) return Value(str, 0, false);
    
    size_t pos = 0;
    while ((pos = str.find(search, pos)) != std::string::npos) {
        str.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    
    return Value(str, 0, false);
}

// toUpperCase(str) -> string
Value string_toUpperCase(std::vector<Value> args) {
    if (args.empty()) return Value("", 0, false);
    
    std::string str = args[0].toString();
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return Value(str, 0, false);
}

// toLowerCase(str) -> string
Value string_toLowerCase(std::vector<Value> args) {
    if (args.empty()) return Value("", 0, false);
    
    std::string str = args[0].toString();
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return Value(str, 0, false);
}

// startsWith(str, prefix) -> boolean
Value string_startsWith(std::vector<Value> args) {
    if (args.size() < 2) return Value("", 0, true);
    
    std::string str = args[0].toString();
    std::string prefix = args[1].toString();
    
    bool result = (str.size() >= prefix.size() && 
                   str.compare(0, prefix.size(), prefix) == 0);
    return Value("", result ? 1 : 0, true);
}

// endsWith(str, suffix) -> boolean
Value string_endsWith(std::vector<Value> args) {
    if (args.size() < 2) return Value("", 0, true);
    
    std::string str = args[0].toString();
    std::string suffix = args[1].toString();
    
    bool result = (str.size() >= suffix.size() && 
                   str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0);
    return Value("", result ? 1 : 0, true);
}

// indexOf(str, search) -> int
Value string_indexOf(std::vector<Value> args) {
    if (args.size() < 2) return Value("", -1, true);
    
    std::string str = args[0].toString();
    std::string search = args[1].toString();
    
    size_t pos = str.find(search);
    return Value("", (pos != std::string::npos) ? (int)pos : -1, true);
}

Value string_find(std::vector<Value> args) {
    return string_indexOf(args);
}

// concat(str1, str2, ...) -> string
Value string_concat(std::vector<Value> args) {
    std::string result;
    for (auto& arg : args) {
        result += arg.toString();
    }
    return Value(result, 0, false);
}

// substring(str, start, end) -> string
Value string_substring(std::vector<Value> args) {
    if (args.empty()) return Value("", 0, false);
    
    std::string str = args[0].toString();
    int start = (args.size() > 1 && args[1].isInt) ? args[1].intVal : 0;
    int end = (args.size() > 2 && args[2].isInt) ? args[2].intVal : str.length();
    
    if (start < 0) start = 0;
    if (end > (int)str.length()) end = str.length();
    if (start >= end) return Value("", 0, false);
    
    return Value(str.substr(start, end - start), 0, false);
}

// length(str) -> int
Value string_length(std::vector<Value> args) {
    if (args.empty()) return Value("", 0, true);
    return Value("", (int)args[0].toString().length(), true);
}

// Register all string functions
void register_string_lib(Interpreter& interp) {
    interp.registerNative("split", string_split);
    interp.registerNative("join", string_join);
    interp.registerNative("trim", string_trim);
    interp.registerNative("replace", string_replace);
    interp.registerNative("toUpperCase", string_toUpperCase);
    interp.registerNative("toLowerCase", string_toLowerCase);
    interp.registerNative("startsWith", string_startsWith);
    interp.registerNative("endsWith", string_endsWith);
    interp.registerNative("indexOf", string_indexOf);
    interp.registerNative("find", string_find);
    interp.registerNative("concat", string_concat);
    interp.registerNative("substring", string_substring);
    interp.registerNative("str_length", string_length);
}
