# Extending Sunda with Native Libraries

Sunda allows you to extend its functionality by writing native C++ libraries and registering them with the interpreter.

## The `Interpreter` API

The `Interpreter` class provides several methods to interact with the Sunda runtime.

### `registerNative(name, func)`
Registers a standalone native function that can be called from Sunda.
- `name`: The name of the function as it will appear in Sunda.
- `func`: A C++ lambda or function with the signature `Value(std::vector<Value> args)`.

### `Value` class
Native functions receive and return `Value` objects.
- `Value(std::string s, int i, bool isI)`: Creates a primitive value.
- `Value(std::vector<Value> list)`: Creates a list.
- `Value(std::map<std::string, Value> map)`: Creates an object/map.
- `Value(NativeFunc func)`: Creates a native function value (used for closures/methods).

### `callClosure(Value, std::vector<Value>)`
Calls a Sunda function/closure from C++.

## How to add a new library

1. **Create a header file** in `lib/your_library/your_library.h`.
2. **Implement your native functions** using the `Value(std::vector<Value> args)` signature.
3. **Define a registration function**:
   ```cpp
   void register_my_lib(Interpreter& interpreter) {
       interpreter.registerNative("my_func", my_func_impl);
   }
   ```
4. **Update `lib/register.cpp`**:
   - Include your header.
   - Call `register_my_lib(interpreter)` inside `register_std_libs`.
5. **Update `core/lang/interpreter.cpp`**:
   - Add your module name to the JIT import list in `Interpreter::execute` for `ImportStmt`.

## Example: Factory Pattern for Objects
If you want to create an "object" with "methods", you can use the factory pattern by returning a Sunda map containing native closures.

```cpp
Value MyLib_create(std::vector<Value> args) {
    std::map<std::string, Value> obj;
    obj["sayHello"] = Value([](std::vector<Value> args) -> Value {
        return Value("Hello from C++!", 0, false);
    });
    return Value(obj);
}
```

In Sunda:
```javascript
import { MyLib_create } from "mylib";
const my = MyLib_create();
my.sayHello(); // "Hello from C++!"
```
