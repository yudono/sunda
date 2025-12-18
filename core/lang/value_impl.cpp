
#include "interpreter.h"
#include <iostream>

// Value Implementation
Value::Value(std::string s, int i, bool isI) : strVal(s), intVal(i), isInt(isI) {}

Value::Value(std::shared_ptr<Stmt> body, std::shared_ptr<Environment> env, std::vector<std::string> params) 
    : strVal("function"), intVal(0), isInt(false), isClosure(true), closureBody(body), closureEnv(env), closureParams(params) {}

Value::Value(std::vector<Value> list) : strVal(""), intVal(0), isInt(false), isList(true) {
    listVal = std::make_shared<std::vector<Value>>(list);
}

Value::Value(std::map<std::string, Value> map) : strVal(""), intVal(0), isInt(false), isMap(true) {
    mapVal = std::make_shared<std::map<std::string, Value>>(map);
}

Value::Value(NativeFunc func) : strVal("native"), intVal(0), isInt(false), nativeFunc(func), isNative(true) {}

Value::Value() : strVal(""), intVal(0), isInt(false) {}

std::string Value::toString() const { 
    if (isClosure) return "[Function]";
    if (isNative) return "[Native Function]";
    if (isList && listVal) {
        std::string s = "[";
        for(size_t i=0; i<listVal->size(); i++) {
            s += (*listVal)[i].toString();
            if (i < listVal->size()-1) s += ", ";
        }
        s += "]";
        return s;
    }
    if (isMap && mapVal) {
        std::string s = "{";
         for(auto const& pair : *mapVal) {
             s += pair.first + ": " + pair.second.toString() + ", ";
         }
         if (mapVal->size() > 0) s = s.substr(0, s.length()-2);
        s += "}";
        return s;
    }
    return isInt ? std::to_string(intVal) : strVal; 
}

std::string Value::toJson() const {
    if (isInt) {
        if (strVal == "true") return "true";
        if (strVal == "false") return "false";
        return std::to_string(intVal);
    }
    if (isList && listVal) {
        // ...
        std::string s = "[";
        for (size_t i = 0; i < listVal->size(); i++) {
            s += (*listVal)[i].toJson();
            if (i < listVal->size() - 1) s += ",";
        }
        s += "]";
        return s;
    }
    if (isMap && mapVal) {
        std::string s = "{";
        size_t i = 0;
        for (auto const& pair : *mapVal) {
            s += "\"" + pair.first + "\":" + pair.second.toJson();
            if (++i < mapVal->size()) s += ",";
        }
        s += "}";
        return s;
    }
    
    if (strVal == "null" || strVal == "undefined") return "null";
    if (isClosure || isNative) return "null";
    
    // Default: string with quotes
    // Basic escaping
    std::string escaped = strVal;
    size_t pos = 0;
    while ((pos = escaped.find("\"", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\"");
        pos += 2;
    }
    return "\"" + escaped + "\"";
}
