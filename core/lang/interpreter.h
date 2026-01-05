#ifndef ANIS_INTERPRETER_H
#define ANIS_INTERPRETER_H

#include "parser.h"
#include <map>
#include <string>
#include <functional>

// Forward Decl
struct Environment;
struct Class;
struct Instance;

struct Value {
    std::string strVal;
    int intVal;
    bool isInt; 
    bool isClosure = false;
    std::shared_ptr<Stmt> closureBody; 
    std::shared_ptr<Environment> closureEnv; // Captured scope
    std::vector<std::string> closureParams; // Added params
    
    // List support (Reference Semantics)
    std::shared_ptr<std::vector<Value>> listVal;
    bool isList = false;

    // Map/Object support (Reference Semantics)
    std::shared_ptr<std::map<std::string, Value>> mapVal;
    bool isMap = false;

    // Native Function
    using NativeFunc = std::function<Value(std::vector<Value>)>;
    NativeFunc nativeFunc;
    bool isNative = false;
    
    // Getter/Setter tagging
    bool isGetter = false;
    bool isSetter = false;
    
    // OOP support (Reference Semantics)
    std::shared_ptr<Class> classVal;
    bool isClass = false;
    std::shared_ptr<Instance> instanceVal;
    bool isInstance = false;
    
    std::string nativeId; // Stable identification for native closures
    
    Value(std::string s, int i, bool isI);
    Value(std::shared_ptr<Stmt> body, std::shared_ptr<Environment> env = nullptr, std::vector<std::string> params = {});
    Value(std::vector<Value> list);
    Value(std::map<std::string, Value> map);
    Value(NativeFunc func);
    Value(std::shared_ptr<Class> c);
    Value(std::shared_ptr<Instance> i);
    Value();
    
    std::string toString() const;
    std::string toJson() const;
    
    // Type safety helper methods
    std::string getTypeName() const {
        if (isInt) return "number";
        if (isList) return "array";
        if (isMap) return "object";
        if (isClosure) return "function";
        if (isNative) return "native function";
        if (isClass) return "class";
        if (isInstance) return "instance";
        if (isGetter) return "getter";
        if (isSetter) return "setter";
        return "string";
    }
    
    bool isCallable() const {
        return isClosure || isNative;
    }
    
    bool isTruthy() const {
        if (isInt) return intVal != 0;
        if (strVal == "false" || strVal == "null" || strVal == "undefined") return false;
        if (isList && listVal) return !listVal->empty();
        if (isMap && mapVal) return !mapVal->empty();
        return !strVal.empty() || isClosure || isNative || isClass || isInstance;
    }
    
    bool isNullOrUndefined() const {
        return (!isInt && (strVal == "null" || strVal == "undefined"));
    }
    
    // Safe accessors with defaults
    int safeGetInt(int defaultVal = 0) const {
        return isInt ? intVal : defaultVal;
    }
    
    std::string safeGetString(const std::string& defaultVal = "") const {
        return !isInt ? strVal : defaultVal;
    }
    
    // Safe list access
    Value safeGetListItem(size_t index, const Value& defaultVal = Value("undefined", 0, false)) const {
        if (isList && listVal && index < listVal->size()) {
            return (*listVal)[index];
        }
        return defaultVal;
    }
    
    // Safe map access
    Value safeGetMapValue(const std::string& key, const Value& defaultVal = Value("undefined", 0, false)) const {
        if (isMap && mapVal) {
            auto it = mapVal->find(key);
            if (it != mapVal->end()) {
                return it->second;
            }
        }
        return defaultVal;
    }
    
    // Check if value is a specific type
    bool checkType(const std::string& expectedType) const {
        return getTypeName() == expectedType;
    }

};

struct Environment {
    std::map<std::string, Value> values;
    std::shared_ptr<Environment> enclosing;
    
    Environment(std::shared_ptr<Environment> enc = nullptr) : enclosing(enc) {}
    
    void define(std::string name, Value v) {
        values[name] = v;
    }
    
    // Assign to nearest scope
    void assign(std::string name, Value v) {
        if (values.count(name)) {
            values[name] = v;
            return;
        }
        if (enclosing) enclosing->assign(name, v);
    }
    
    // Get from nearest scope
    Value get(std::string name) {
        if (values.count(name)) return values[name];
        if (enclosing) return enclosing->get(name);
        return {"undefined", 0, false};
    }
};


struct Class {
    std::string name;
    std::shared_ptr<Class> superclass;
    std::map<std::string, Value> methods;
    std::map<std::string, Value> getters;
    std::map<std::string, Value> setters;
    std::map<std::string, Value> staticFields;
    std::map<std::string, std::shared_ptr<Expr>> instanceFields;
    std::vector<std::string> privateFieldNames;
    
    Class(std::string n, std::shared_ptr<Class> s = nullptr) : name(n), superclass(s) {}
    Value findMethod(const std::string& name);
    Value findGetter(const std::string& name);
    Value findSetter(const std::string& name);
};

struct Instance {
    std::shared_ptr<Class> klass;
    std::map<std::string, Value> fields;
    std::map<std::string, Value> privateFields;
    
    Instance(std::shared_ptr<Class> k) : klass(k) {}
    Value get(const std::string& name);
    void set(const std::string& name, Value value);
};

class Interpreter {
public:
    std::shared_ptr<Environment> globals;
    std::shared_ptr<Environment> environment; // Current scope
    
    std::map<std::string, std::function<Value(std::vector<Value>)>> natives;
    
    Value lastReturnValue;
    bool isReturning = false;
    
    Value lastExpressionValue;
    bool hasLastExpressionValue = false;
    
    // Hooks State
    std::vector<Value> hooks;
    int hookIndex = 0;
     
    std::string sourceCode; // For debug
    std::string currentFile = "main.anis"; // Default
    int currentLine = 0;
    
    void resetHooks() { hookIndex = 0; }

public:
    // Helpers
   void setVar(std::string name, Value v);
   Value getVar(std::string name);

    Interpreter();
    void interpret(const std::vector<std::shared_ptr<Stmt>>& statements);
    void registerNative(std::string name, std::function<Value(std::vector<Value>)> func);
    
    // API for host
    Value getGlobal(std::string name);
    void callFunction(std::string name);
    Value callClosure(Value closure, std::vector<Value> args = {}); 
    void executeClosure(Value closure, std::vector<Value> args = {}); 
    
private:
    void execute(std::shared_ptr<Stmt> stmt);
    Value evaluate(std::shared_ptr<Expr> expr);
    bool isTrue(Value v) const;
    
    void executeBlock(std::shared_ptr<BlockStmt> block, std::shared_ptr<Environment> env);

};


// Exception Support
struct RuntimeError : public std::runtime_error {
    Value value; // The thrown value (can be string or Error object)
    RuntimeError(Value v) : std::runtime_error(v.toString()), value(v) {}
};

#endif
