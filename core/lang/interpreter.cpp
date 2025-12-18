#include "interpreter.h"
#include <iostream>
#include "../debugger.h"
#include "../../lib/http/http_lib.h"

Interpreter::Interpreter() {
    globals = std::make_shared<Environment>();
    environment = globals;
    
    // Default natives
    auto print = [](std::vector<Value> args) {
        for(auto& a : args) std::cout << a.toString();
        return Value("", 0, true);
    };
    globals->define("print", Value(print));
    natives["print"] = print; // Keep for backward compat if needed?

    auto println = [](std::vector<Value> args) {
        for(auto& a : args) std::cout << a.toString();
        std::cout << std::endl;
        return Value("", 0, true);
    };
    globals->define("println", Value(println));
    natives["println"] = println;

    // Default literals
    globals->define("true", {"true", 1, true});
    globals->define("false", {"false", 0, true});
    globals->define("null", {"null", 0, false});
    globals->define("undefined", {"undefined", 0, false});
}

void Interpreter::setVar(std::string name, Value v) {
    // If not found in chain, define in globals? 
    // Or closer scope?
    // Interpreter Logic: assign if exists, else define in current?
    // User requested behavior: "var count" defines it. "count = 1" assigns.
    // If we just use setVar for both?
    // Current Sunda: `varDecl` uses define. `assignment` uses assign.
    
    // Helper: this function is for assignment (update)
    environment->assign(name, v);
    // If assign failed (env didn't find it), what happens?
    // My Environment::assign does recursion. If root doesn't have it?
    // It does nothing.
    // We should probably check or callback error.
    // However, for MVP, we might want implicit global definition if missing?
    // Let's stick to explicit `define` in VarDecl.
}

Value Interpreter::getVar(std::string name) {
    return environment->get(name);
}

void Interpreter::registerNative(std::string name, std::function<Value(std::vector<Value>)> func) {
    natives[name] = func;
}

void Interpreter::interpret(const std::vector<std::shared_ptr<Stmt>>& statements) {
    for (auto& s : statements) {
        if (s) execute(s);
    }
}

// Forward declaration of render_gui bridging helper if needed?
#include "interpreter.h"
#include "lexer.h"
#include <iostream>
#include <fstream>
#include <sstream>

void Interpreter::execute(std::shared_ptr<Stmt> stmt) {
    if (isReturning) return;

    if (auto imp = std::dynamic_pointer_cast<ImportStmt>(stmt)) {
        // JIT Loading
        // std::cout << "[JIT] Loading module: " << imp->moduleName << std::endl;
        
        if (imp->moduleName == "gui" || imp->moduleName == "math" || imp->moduleName == "string" || 
            imp->moduleName == "array" || imp->moduleName == "map" || imp->moduleName == "db" || 
            imp->moduleName == "webserver" || imp->moduleName == "fs" || imp->moduleName == "os" || 
            imp->moduleName == "exec" || imp->moduleName == "regex" || imp->moduleName == "json" || 
            imp->moduleName == "http") {
            // Built-in module: import requested symbols
            if (!imp->symbols.empty()) {
                // Import specific symbols: import { render_gui } from "gui"
                for (auto& sym : imp->symbols) {
                    if (natives.count(sym)) {
                        // Create a Value wrapper for the native function
                        Value nativeVal(natives[sym]);
                        globals->define(sym, nativeVal);
                    }
                }
            }
            return;
        }

        // File loading
        std::string filename = imp->moduleName;
        // User requested .sd extension, support .s (legacy) and .sd
        if (filename.find('.') == std::string::npos && filename.find("://") == std::string::npos) {
            filename += ".sd";
        }
        
        std::string source;
        bool loaded = false;

        // Remote Import detection
        if (filename.find("http://") == 0 || filename.find("https://") == 0) {
            // std::cout << "[Remote Import] Fetching: " << filename << std::endl;
            source = HTTPLib::fetch(filename);
            if (!source.empty()) {
                loaded = true;
            } else {
                Debugger::runtimeError("Failed to fetch remote module: " + filename, 0);
                return;
            }
        } else {
            // Resolve relative paths using g_basePath
            extern std::string g_basePath;
            std::string fullPath = filename;
            if (!filename.empty() && filename[0] == '.') {
                // Remove ./ prefix
                if (filename.length() >= 2 && filename[1] == '/') {
                    fullPath = filename.substr(2);  // Remove "./"
                }
                fullPath = g_basePath + fullPath;
            }
            
            std::ifstream file(fullPath);
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                source = buffer.str();
                loaded = true;

                // Extract directory from filename for nested imports
                // Only for local files
                extern std::string g_basePath;
                size_t lastSlash = fullPath.find_last_of('/');
                if (lastSlash != std::string::npos) {
                    g_basePath = fullPath.substr(0, lastSlash + 1);
                }
            }
        }

        if (loaded) {
            // Store source for debugging
            this->sourceCode = source;
            
            Lexer lexer(source);
            auto tokens = lexer.tokenize();
            Parser parser(tokens);
            auto stmts = parser.parse();
            
            // For remote imports, we should probably restore g_basePath if it was changed
            // but we only change it for local files now.
            interpret(stmts);
        } else {
             Debugger::runtimeError("Could not find module '" + imp->moduleName + "'", 0);
        }
        return;
    }
    
    if (auto dest = std::dynamic_pointer_cast<DestructureStmt>(stmt)) {
        Value init = evaluate(dest->initializer);
        if (init.isList && init.listVal && init.listVal->size() >= dest->names.size()) {
             for (size_t i = 0; i < dest->names.size(); i++) {
                 environment->define(dest->names[i], (*init.listVal)[i]);
             }
         } else {
             Debugger::runtimeError("Destructuring mismatch or not a list. Initializer type: " + std::to_string(init.isList) + ", Size: " + (init.listVal ? std::to_string(init.listVal->size()) : "null"), 0);
         }
    }
    else if (auto exp = std::dynamic_pointer_cast<ExportStmt>(stmt)) {
        // Execute the declaration (function, var, etc)
        execute(exp->declaration);
        // TODO: Store in module exports map for import system
    }
    else if (auto varDecl = std::dynamic_pointer_cast<VarDeclStmt>(stmt)) {
        Value val = {"", 0, true};
        if (varDecl->initializer) val = evaluate(varDecl->initializer);
        environment->define(varDecl->name, val); // Define in current scope
    }
    else if (auto ret = std::dynamic_pointer_cast<ReturnStmt>(stmt)) {
         lastReturnValue = evaluate(ret->value);
         isReturning = true;
    }
    else if (auto funcDecl = std::dynamic_pointer_cast<FuncDeclStmt>(stmt)) {
        // Store as Value (Closure) in GLOBAL scope (top-level functions should be global)
        // Capture CURRENT environment and Params
        globals->define(funcDecl->name, Value(funcDecl->body, environment, funcDecl->params));
    }
    else if (auto block = std::dynamic_pointer_cast<BlockStmt>(stmt)) {
        executeBlock(block, std::make_shared<Environment>(environment));
    }
    else if (auto ifStmt = std::dynamic_pointer_cast<IfStmt>(stmt)) {
        Value cond = evaluate(ifStmt->condition);
        if (isTrue(cond)) execute(ifStmt->thenBranch);
        else if (ifStmt->elseBranch) execute(ifStmt->elseBranch);
    }
    else if (auto whileStmt = std::dynamic_pointer_cast<WhileStmt>(stmt)) {
        while (true) {
            Value cond = evaluate(whileStmt->condition);
            if (!isTrue(cond)) break;
            execute(whileStmt->body);
            if (isReturning) break;  // Handle early return
        }
    }
    else if (auto switchStmt = std::dynamic_pointer_cast<SwitchStmt>(stmt)) {
        Value val = evaluate(switchStmt->condition);
        bool matchFound = false;
        
        // Helper to execute case body (which is Stmt, not Value)
        auto execStmt = [&](std::shared_ptr<Stmt> body) {
             if (auto block = std::dynamic_pointer_cast<BlockStmt>(body)) {
                 // For switch cases in same scope, we might just executeBlock? 
                 // Or create scope?
                 // Standard is: Switch shares scope or block scope per case?
                 // Let's create block scope.
                 executeBlock(block, std::make_shared<Environment>(environment));
             }
        };
        
        for (auto& cs : switchStmt->cases) {
            if (cs.value) { // Case
                Value caseVal = evaluate(cs.value);
                bool eq = false;
                if (val.isInt && caseVal.isInt) eq = (val.intVal == caseVal.intVal);
                else if (!val.isInt && !caseVal.isInt) eq = (val.strVal == caseVal.strVal);
                
                if (eq) {
                    execStmt(cs.body); 
                    matchFound = true;
                    break;
                }
            }
        }
        
        if (!matchFound) {
            for (auto& cs : switchStmt->cases) {
                if (!cs.value) { 
                     execStmt(cs.body);
                     break;
                }
            }
        }
    }
    else if (auto exprStmt = std::dynamic_pointer_cast<ExprStmt>(stmt)) {
        lastExpressionValue = evaluate(exprStmt->expr);
        hasLastExpressionValue = true;
    }
}




void Interpreter::executeBlock(std::shared_ptr<BlockStmt> block, std::shared_ptr<Environment> env) {
    std::shared_ptr<Environment> previous = environment;
    environment = env;
    
    for (auto& s : block->statements) {
        execute(s);
        if (isReturning) break;
    }
    
    environment = previous;
}

Value Interpreter::evaluate(std::shared_ptr<Expr> expr) {
    if (expr->line > 0) currentLine = expr->line;
    if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(expr)) {
        if (lit->isString) return {lit->value, 0, false};
        return {"", std::stoi(lit->value), true};
    }
    if (auto var = std::dynamic_pointer_cast<VarExpr>(expr)) {
        return getVar(var->name);
    }
    if (auto unary = std::dynamic_pointer_cast<UnaryExpr>(expr)) {
        Value right = evaluate(unary->right);
        if (unary->op == "!") return {"", !isTrue(right) ? 1 : 0, true};
        if (unary->op == "-" && right.isInt) return {"", -right.intVal, true};
        return right;
    }
    if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
        Value callee = evaluate(call->callee);
        
        std::vector<Value> args;
        for (auto& arg : call->args) {
            args.push_back(evaluate(arg));
        }
        
        if (callee.isNative) {
            return callee.nativeFunc(args);
        }
        
        if (callee.isClosure) {
             return callClosure(callee, args);
        }
        
        std::string name = "expression";
        if (auto var = std::dynamic_pointer_cast<VarExpr>(call->callee)) {
            name = "'" + var->name + "'";
        }
        Debugger::runtimeError("Attempt to call non-function: " + name + " is " + callee.toString(), currentLine, sourceCode, currentFile);
        return {"", 0, true}; // Unreachable
    }
    if (auto ternary = std::dynamic_pointer_cast<TernaryExpr>(expr)) {
        Value cond = evaluate(ternary->condition);
        if (isTrue(cond)) {
            return evaluate(ternary->trueExpr);
        } else {
            return evaluate(ternary->falseExpr);
        }
    }
    if (auto bin = std::dynamic_pointer_cast<BinaryExpr>(expr)) {
        if (bin->op == "+=") {
            if (auto var = std::dynamic_pointer_cast<VarExpr>(bin->left)) {
                 Value r = evaluate(bin->right);
                 Value l = getVar(var->name); 
                 if (l.isInt && r.isInt) {
                     Value newVal("", l.intVal + r.intVal, true);
                     setVar(var->name, newVal);
                     return newVal;
                 }
            }
        }
        if (bin->op == "=") {
            if (auto var = std::dynamic_pointer_cast<VarExpr>(bin->left)) {
                 Value val = evaluate(bin->right);
                 setVar(var->name, val);
                 return val;
            }
            if (auto mem = std::dynamic_pointer_cast<MemberExpr>(bin->left)) {
                Value obj = evaluate(mem->object);
                Value val = evaluate(bin->right);
                std::string key;
                
                if (mem->computed) {
                    Value k = evaluate(mem->property);
                    key = k.toString();
                } else {
                    if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(mem->property)) key = lit->value;
                }
                
                if (obj.isMap && obj.mapVal) {
                    (*obj.mapVal)[key] = val;
                    return val;
                }
            }
        }
        
        // Short-circuit logic
        if (bin->op == "&&") {
             Value l = evaluate(bin->left);
             if (!isTrue(l)) return l; // Short-circuit false
             return evaluate(bin->right);
        }
        if (bin->op == "||") {
             Value l = evaluate(bin->left);
             if (isTrue(l)) return l; // Short-circuit true
             return evaluate(bin->right);
        }

        Value l = evaluate(bin->left);
        Value r = evaluate(bin->right);
        
        if (bin->op == "==") {
             bool eq = (l.isInt == r.isInt) && (l.intVal == r.intVal) && (l.strVal == r.strVal);
             return {"", eq ? 1 : 0, true};
        }
        if (bin->op == "!=") {
             bool neq = !((l.isInt == r.isInt) && (l.intVal == r.intVal) && (l.strVal == r.strVal));
             return {"", neq ? 1 : 0, true};
        }
        if (bin->op == "<") {
             if (l.isInt && r.isInt) return {"", (l.intVal < r.intVal) ? 1 : 0, true};
             return {"", 0, true};
        }
        if (bin->op == ">") {
             if (l.isInt && r.isInt) return {"", (l.intVal > r.intVal) ? 1 : 0, true};
             return {"", 0, true};
        }
        if (bin->op == "<=") {
             if (l.isInt && r.isInt) return {"", (l.intVal <= r.intVal) ? 1 : 0, true};
             return {"", 0, true};
        }
        if (bin->op == ">=") {
             if (l.isInt && r.isInt) return {"", (l.intVal >= r.intVal) ? 1 : 0, true};
             return {"", 0, true};
        }
        if (bin->op == "+") {
             if (l.isInt && r.isInt) return {"", l.intVal + r.intVal, true};
             return {l.toString() + r.toString(), 0, false};
        }
        if (bin->op == "-") {
             if (l.isInt && r.isInt) return {"", l.intVal - r.intVal, true};
             return {"", 0, true};
        }
        if (bin->op == "*") {
             if (l.isInt && r.isInt) return {"", l.intVal * r.intVal, true};
             return {"", 0, true};
        }
        if (bin->op == "/") {
             if (l.isInt && r.isInt && r.intVal != 0) return {"", l.intVal / r.intVal, true};
             return {"", 0, true};
        }

    }
    
    // Function Expression (Lambda)
    if (auto func = std::dynamic_pointer_cast<FunctionExpr>(expr)) {
        return Value(func->body, environment, func->params);
    }
    
    if (auto jsx = std::dynamic_pointer_cast<JsxExpr>(expr)) {
        // Component Expansion?
        // Check if tagName is a variable (function) in scope
        Value v = getVar(jsx->tagName);
        if (v.isClosure) {
             // User Component
             // Capture props
             std::map<std::string, Value> props;
             for (auto const& attr : jsx->attributes) {
                  props[attr.first] = evaluate(attr.second);
             }
             
             // Call it with props
             Value ret = callClosure(v, { Value(props) }); // Use Value
             return ret;
         }
         
         std::string xml = "<" + jsx->tagName;
        for (auto const& attr : jsx->attributes) {
             std::string key = attr.first;
             Value attrVal = evaluate(attr.second);
             if (key.substr(0, 2) == "on" && attrVal.isClosure) {
                 std::string id = "cb_" + std::to_string((uintptr_t)attrVal.closureBody.get());
                 if (attrVal.isNative && !attrVal.nativeId.empty()) {
                     id = attrVal.nativeId;
                 }

                 // Use bind_native_input for onInput, bind_native_click for others
                 if (key == "onInput" && natives.count("bind_native_input")) {
                     natives["bind_native_input"]({Value(id, 0, false), attrVal});
                 } else if (natives.count("bind_native_click")) {
                     natives["bind_native_click"]({Value(id, 0, false), attrVal});
                 }
                 
                 xml += " " + key + "=\"" + id + "\"";
                 continue;
             }
             xml += " " + key + "=\"" + attrVal.toString() + "\"";
         }
        // ... children logic
        if (jsx->children.empty()) {
            xml += " />";
        } else {
            xml += ">";
            for(auto c : jsx->children) {
                 Value cv = evaluate(c);
                //  std::cerr << "JSX CHILD: isList=" << cv.isList << " isInt=" << cv.isInt << " isClosure=" << cv.isClosure << " isNative=" << cv.isNative << " str=" << cv.strVal.substr(0, 50) << std::endl;
                 // Skip falsy values (for conditional rendering: {condition && <Component />})
                 // IMPORTANT: Lists should NOT be treated as falsy even if strVal is empty!
                 bool isFalsy = (cv.isInt && cv.intVal == 0) || (!cv.isInt && !cv.isList && cv.strVal.empty());
                 if (!isFalsy) {
                     // Check if it is a list (result of map)
                     if (cv.isList && cv.listVal) {
                         // Flatten list
                        //  std::cerr << "FLATTEN: List size=" << cv.listVal->size() << std::endl;
                         for (const auto& item : *cv.listVal) {
                             xml += item.toString();
                            //  std::cerr << "FLATTEN ITEM: " << item.toString().substr(0, 100) << "..." << std::endl;
                         }
                     } else {
                         xml += cv.toString();
                     }
                 }
            }
            xml += "</" + jsx->tagName + ">";
        }
        return {xml, 0, false};
    }
    
    // Objects/Arrays
    if (auto obj = std::dynamic_pointer_cast<ObjectExpr>(expr)) {
        std::map<std::string, Value> map;
        for (auto const& prop : obj->properties) {
            // Check if this is a spread property
            if (prop.first.find("__spread_") == 0) {
                if (auto spread = std::dynamic_pointer_cast<SpreadExpr>(prop.second)) {
                    Value spreadVal = evaluate(spread->argument);
                    // Merge spread object properties into current object
                    if (!spreadVal.isInt && spreadVal.mapVal) {
                        for (auto const& kv : *spreadVal.mapVal) {
                            map[kv.first] = kv.second;
                        }
                    }
                }
            } else {
                map[prop.first] = evaluate(prop.second);
            }
        }
        return Value(map);
    }
    if (auto arr = std::dynamic_pointer_cast<ArrayExpr>(expr)) {
        std::vector<Value> list;
        for (auto const& e : arr->elements) {
            // Check if this is a spread expression
            if (auto spread = std::dynamic_pointer_cast<SpreadExpr>(e)) {
                Value spreadVal = evaluate(spread->argument);
                if (spreadVal.isList && spreadVal.listVal) {
                    // Spread the array elements
                    for (auto& item : *spreadVal.listVal) {
                        list.push_back(item);
                    }
                } else if (spreadVal.isMap && spreadVal.mapVal) {
                    // Spread object (future: for object literals)
                    Debugger::warning("Spread of objects in arrays not yet supported", "", 0);
                }
            } else {
                list.push_back(evaluate(e));
            }
        }
        return Value(list);
    }
    if (auto mem = std::dynamic_pointer_cast<MemberExpr>(expr)) {
        Value obj = evaluate(mem->object);
        std::string key;
        if (mem->computed) {
            Value k = evaluate(mem->property);
            key = k.toString();
        } else {
            if (auto lit = std::dynamic_pointer_cast<LiteralExpr>(mem->property)) key = lit->value;
        }
        
        // DEBUG
        // std::cout << "DEBUG: MemberExpr obj.isList=" << obj.isList << " key=" << key << " line=" << currentLine << std::endl;
        
        // List Methods
        if (obj.isList && obj.listVal) {
             if (key == "length") return Value("", (int)obj.listVal->size(), true);
             
             if (key == "push") {
                 Value method([obj](std::vector<Value> args) mutable -> Value {
                     for(auto& a : args) obj.listVal->push_back(a);
                     return Value("", (int)obj.listVal->size(), true);
                 });
                 method.isNative = true;
                 return method;
             }
             
             if (key == "filter") {
                 Value method([obj, this](std::vector<Value> args) mutable -> Value {
                     if (args.empty() || !args[0].isClosure) return Value(std::vector<Value>{});
                     Value callback = args[0];
                     std::vector<Value> result;
                     for(auto& item : *obj.listVal) {
                         Value ret = this->callClosure(callback, {item});
                         if ((ret.isInt && ret.intVal != 0) || (!ret.isInt && !ret.strVal.empty())) {
                             result.push_back(item);
                         }
                     }
                     return Value(result);
                 });
                 method.isNative = true;
                 return method;
             }
             
             if (key == "map") {
                 Value method([obj, this](std::vector<Value> args) mutable -> Value {
                     if (args.empty() || !args[0].isClosure) return Value(std::vector<Value>{});
                     Value callback = args[0];
                     std::vector<Value> result;
                     for(auto& item : *obj.listVal) {
                         Value ret = this->callClosure(callback, {item});
                         result.push_back(ret);
                     }
                     return Value(result);
                 });
                 method.isNative = true;
                 return method;
             }
        }
        
        if (obj.isMap && obj.mapVal) {
            if (obj.mapVal->count(key)) return (*obj.mapVal)[key];
        }
        if (obj.isList && obj.listVal) {
             // Array Properties
             if (key == "length") {
                 return Value("", (int)obj.listVal->size(), true);
             }

             // Array Methods
             if (key == "map") {
                 return Value([this, obj](std::vector<Value> args) -> Value {
                     if (args.empty() || !args[0].isClosure) return Value(std::vector<Value>{});
                     Value cb = args[0];
                     std::vector<Value> res;
                     for (auto& item : *obj.listVal) {
                         // Call closure with item
                         res.push_back(this->callClosure(cb, {item}));
                     }
                     return Value(res);
                 });
             }
             if (key == "filter") {
                 return Value([this, obj](std::vector<Value> args) -> Value {
                     if (args.empty() || !args[0].isClosure) return Value(std::vector<Value>{});
                     Value cb = args[0];
                     std::vector<Value> res;
                     for (auto& item : *obj.listVal) {
                         Value ret = this->callClosure(cb, {item});
                         bool keep = (ret.isInt && ret.intVal != 0) || (!ret.isInt && !ret.strVal.empty());
                         if (keep) res.push_back(item);
                     }
                     return Value(res);
                 });
             }
             if (key == "push") {
                 return Value([obj](std::vector<Value> args) -> Value {
                     for(auto& a : args) {
                         obj.listVal->push_back(a);
                     }
                     return Value("", (int)obj.listVal->size(), true);
                 });
             }
             if (key == "pop") {
                 return Value([obj](std::vector<Value> args) -> Value {
                     if (obj.listVal->empty()) return {"undefined", 0, false};
                     Value v = obj.listVal->back();
                     obj.listVal->pop_back();
                     return v;
                 });
             }
             // Array Access
             if (isdigit(key[0])) {
                 int idx = std::stoi(key);
                 if (idx >= 0 && idx < obj.listVal->size()) return (*obj.listVal)[idx];
             }
        }
        return {"undefined", 0, false};
    }

    return {"", 0, true};
}

// New method to replace callClosure logic properly
Value Interpreter::callClosure(Value closure, std::vector<Value> args) {
    if (!closure.isClosure || !closure.closureBody) return {"", 0, false};
    
    if (auto block = std::dynamic_pointer_cast<BlockStmt>(closure.closureBody)) {
        // Prepare Environment
        std::shared_ptr<Environment> prev = environment;
        // Use captured env as parent, create new scope
        if (closure.closureEnv) {
            environment = std::make_shared<Environment>(closure.closureEnv);
        } else {
            // Fallback (shouldn't happen if we capture correctly)
            environment = std::make_shared<Environment>(globals); 
        }
        
        // Bind Params
        for (size_t i = 0; i < closure.closureParams.size() && i < args.size(); i++) {
            std::string param = closure.closureParams[i];
            // Check if this is a destructuring pattern
            bool isDestruct = (param.length() > 12 && param.substr(0, 12) == "__destruct:{") || (param.front() == '{' && param.back() == '}');
            if (isDestruct) {
                // Extract property names form pattern
                std::string pattern;
                if (param.front() == '{') pattern = param.substr(1, param.length() - 2);
                else pattern = param.substr(12, param.length() - 13);
                
                std::vector<std::string> props;
                size_t start = 0;
                size_t comma = pattern.find(',');
                while (comma != std::string::npos) {
                    props.push_back(pattern.substr(start, comma - start));
                    start = comma + 1;
                    comma = pattern.find(',', start);
                }
                if (start < pattern.length()) {
                    props.push_back(pattern.substr(start));
                }
                
                // Destructure the argument (should be an object)
                Value arg = args[i];
                if (arg.isMap && arg.mapVal) {
                    for (auto& propName : props) {
                        if (arg.mapVal->count(propName)) {
                            environment->define(propName, (*arg.mapVal)[propName]);
                        } else {
                            environment->define(propName, Value("undefined", 0, false));
                        }
                    }
                }
            } else {
                environment->define(param, args[i]);
            }
        }
        
        isReturning = false;
        executeBlock(block, environment); 
        
        Value ret = {"", 0, true};
        if (isReturning) ret = lastReturnValue;
        isReturning = false;
        
        environment = prev; // Restore
        return ret;
    }
    return {"", 0, true};
}

void Interpreter::executeClosure(Value closure, std::vector<Value> args) {
    if (!closure.isClosure || !closure.closureBody) return;
    
    if (auto block = std::dynamic_pointer_cast<BlockStmt>(closure.closureBody)) {
        std::shared_ptr<Environment> prev = environment;
        if (closure.closureEnv) {
            environment = std::make_shared<Environment>(closure.closureEnv);
        } else {
            environment = std::make_shared<Environment>(globals);
        }
        
        // Bind Params
        for (size_t i = 0; i < closure.closureParams.size() && i < args.size(); i++) {
            std::string param = closure.closureParams[i];
            // Check if this is a destructuring pattern
            if (param.length() > 12 && param.substr(0, 12) == "__destruct:{") {
                // Extract property names from pattern
                std::string pattern = param.substr(12, param.length() - 13);
                std::vector<std::string> props;
                size_t start = 0;
                size_t comma = pattern.find(',');
                while (comma != std::string::npos) {
                    props.push_back(pattern.substr(start, comma - start));
                    start = comma + 1;
                    comma = pattern.find(',', start);
                }
                if (start < pattern.length()) {
                    props.push_back(pattern.substr(start));
                }
                
                // Destructure the argument
                Value arg = args[i];
                if (arg.isMap && arg.mapVal) {
                    for (auto& propName : props) {
                        if (arg.mapVal->count(propName)) {
                            environment->define(propName, (*arg.mapVal)[propName]);
                        } else {
                            environment->define(propName, Value("undefined", 0, false));
                        }
                    }
                }
            } else {
                environment->define(param, args[i]);
            }
        }
        
        executeBlock(block, environment);
        
        environment = prev;
    }
}

Value Interpreter::getGlobal(std::string name) {
    return globals->get(name);
}

void Interpreter::callFunction(std::string name) {
    // Deprecated
}

bool Interpreter::isTrue(Value v) const {
    if (v.isInt) return v.intVal != 0;
    if (v.isList && v.listVal) return true;
    if (v.isMap && v.mapVal) return true;
    if (v.strVal == "undefined" || v.strVal == "null" || v.strVal == "false" || v.strVal == "0") return false;
    return !v.strVal.empty();
}

