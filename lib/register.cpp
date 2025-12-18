#include "../core/lang/interpreter.h"
#include "gui/minigui.h" // For render_gui binding logic which might be moved here later? 
// Actually gui bindings were in sunda.cpp. We should move them here OR keep them separate?
// User asked to move explicit registration to register.cpp

// Include Libs
// Include Libs
#include "math/math.h"
#include "gui/gui_lib.h"
#include "date/date_lib.h"
#include "string/string.h"
#include "array/array.h"
#include "map/map.h"
#include "database/database_lib.h"
#include "webserver/webserver_lib.h"
#include "fs/fs_lib.h"
#include "os/os_lib.h"
#include "exec/exec_lib.h"
#include "regex/regex_lib.h"
#include "json/json_lib.h"
#include "register.h"

// Forward declare GUI registration if it's still in sunda.cpp or move it?
// For now, let's implement a clean registry function.

// Header
void register_std_libs(Interpreter& interpreter) {
    // Math
    MathLib::register_math(interpreter);
    
    // GUI
    GuiLib::register_gui(interpreter);
    
    // Date
    DateLib::register_date(interpreter);
    
    // String
    register_string_lib(interpreter);
    
    // Array
    register_array_lib(interpreter);
    
    // Map
    register_map_lib(interpreter);

    // Database
    DatabaseLib::register_db(interpreter);

    // WebServer
    WebServer::register_webserver(interpreter);

    // File System
    FSLib::register_fs(interpreter);

    // OS
    OSLib::register_os(interpreter);

    // Exec
    ExecLib::register_exec(interpreter);

    // Regex
    RegexLib::register_regex(interpreter);

    // JSON
    JSONLib::register_json(interpreter);
}
