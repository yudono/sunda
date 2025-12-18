#ifndef SUNDA_STRING_LIB_H
#define SUNDA_STRING_LIB_H

#include "../../core/lang/interpreter.h"
#include <string>
#include <vector>

// String manipulation functions
Value string_split(std::vector<Value> args);
Value string_join(std::vector<Value> args);
Value string_trim(std::vector<Value> args);
Value string_replace(std::vector<Value> args);
Value string_toUpperCase(std::vector<Value> args);
Value string_toLowerCase(std::vector<Value> args);
Value string_startsWith(std::vector<Value> args);
Value string_endsWith(std::vector<Value> args);
Value string_indexOf(std::vector<Value> args);
Value string_find(std::vector<Value> args); // Alias for indexOf
Value string_concat(std::vector<Value> args);
Value string_substring(std::vector<Value> args);
Value string_length(std::vector<Value> args);

// Registration function
void register_string_lib(Interpreter& interp);

#endif
