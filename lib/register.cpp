#include "../core/lang/interpreter.h"
#include "gui/minigui.h" // For render_gui binding logic which might be moved here later? 
// Actually gui bindings were in sunda.cpp. We should move them here OR keep them separate?
// User asked to move explicit registration to register.cpp

// Include Libs
// Include Libs
#include "math/math.h"
#include "gui/gui_lib.h"
#include "date/date_lib.h"
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
}
