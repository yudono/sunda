#include "core/lang/interpreter.h"
#include "lib/gui/minigui.h"
#include <iostream>

struct GuiLib {
    static void register_gui(Interpreter& interpreter) {
         // 1. Bind Click (Generic)
         interpreter.registerNative("bind_native_click", [&](std::vector<Value> args) {
              if (args.size() >= 2 && args[1].isClosure) {
                  std::string id = args[0].strVal;
                  Value v = args[1]; // Capture Value
                  bind_click(id, [id, v, &interpreter]() { 
                      interpreter.executeClosure(v); 
                      request_rerender(); 
                  });
              }
              return Value("", 0, true);
         });
         
         // 2. Update Hook Native
         interpreter.registerNative("updateHook", [&](std::vector<Value> args) {
             if (args.size() >= 2) {
                 int idx = args[0].intVal;
                 Value newVal = args[1];
                 if (idx >= 0 && idx < interpreter.hooks.size()) {
                     interpreter.hooks[idx] = newVal;
                     request_rerender();
                 }
             }
             return Value("", 0, true);
         });
         
         // 3. Bridge setState (Hooks Style)
         interpreter.registerNative("setState", [&](std::vector<Value> args) {
              int idx = interpreter.hookIndex++;
              if (idx >= interpreter.hooks.size()) {
                  if (args.empty()) {
                      interpreter.hooks.push_back(Value("undefined", 0, false));
                  } else {
                      interpreter.hooks.push_back(args[0]);
                  }
              }
              Value currentVal = interpreter.hooks[idx];
              
              Value setterClosure(std::make_shared<BlockStmt>(), nullptr, std::vector<std::string>{"newVal"});
              setterClosure.isClosure = true;
              setterClosure.isNative = true;
              setterClosure.nativeFunc = [idx, &interpreter](std::vector<Value> innerArgs) {
                   if (innerArgs.size() > 0) {
                       interpreter.hooks[idx] = innerArgs[0];
                       request_rerender();
                   }
                   return Value("", 0, true);
              };
              
              // Return [value, setter]
              std::vector<Value> retList;
              retList.push_back(currentVal);
              retList.push_back(setterClosure);
              return Value(retList);
         });

         // 4. Render GUI
         interpreter.registerNative("render_gui", [&](std::vector<Value> args) {
             std::cout << "Starting GUI from Sunda..." << std::endl;
             if (args.size() > 0 && args[0].isClosure) {
                  Value component = args[0]; // The App function
                  
                  // Initialize Minigui with Main Loop
                  render_gui([component, &interpreter]() -> std::string {
                      interpreter.hookIndex = 0; // Reset hooks for render pass
                      // interpreter.env_stack.clear(); // env_stack is not public? check interpreter.h
                      // Actually, executeClosure creates new env scope.
                      // If we clear env_stack, we might lose globals? No, env_stack likely local.
                      // Let's remove env_stack logic if it's suspicious or check interpreter.h
                      // For now, assume it's not needed or use provided logic.
                      
                      // Execute App()
                      Value v = interpreter.callClosure(component, std::vector<Value>{});
                      return v.toString();
                  });
             }
             return Value("", 0, true);
         });
    }
};
