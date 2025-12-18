# Detect Operating System
ifeq ($(OS),Windows_NT)
    OS_NAME = windows
    EXE_EXT = .exe
else
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Darwin)
        OS_NAME = macos
    else
        OS_NAME = linux
    endif
    EXE_EXT =
endif

# Compiler and Basic Flags
CXX = clang++
CXXFLAGS = -std=c++17 -Wall -O2 -Wno-deprecated-declarations -Wno-c++11-narrowing

# OS-specific settings
ifeq ($(OS_NAME),macos)
    HOMEBREW_PREFIX = /opt/homebrew
    OPENSSL_PREFIX = /opt/homebrew/opt/openssl@3
    INCLUDES = -I$(HOMEBREW_PREFIX)/include -I$(HOMEBREW_PREFIX)/include/freetype2 -I$(HOMEBREW_PREFIX)/include/mysql -I$(OPENSSL_PREFIX)/include
    LDFLAGS = -L$(HOMEBREW_PREFIX)/lib -L$(OPENSSL_PREFIX)/lib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lglfw -lfreetype -lsqlite3 -lmysqlclient -lcurl -lssl -lcrypto
else ifeq ($(OS_NAME),linux)
    INCLUDES = $(shell pkg-config --cflags glfw3 freetype2 sqlite3 mysqlclient libcurl openssl)
    LDFLAGS = $(shell pkg-config --libs glfw3 freetype2 sqlite3 mysqlclient libcurl openssl) -lGL -lX11 -lpthread -ldl
else ifeq ($(OS_NAME),windows)
    # Windows build - assumes libraries installed via MSYS2/MinGW
    # DLL files in win/ are for runtime only
    INCLUDES = -I/mingw64/include -I/mingw64/include/freetype2 -I/mingw64/include/mysql
    LDFLAGS = -L/mingw64/lib -lglfw3 -lfreetype -lsqlite3 -lmysqlclient -lcurl -lssl -lcrypto -lgdi32 -lopengl32 -lwinmm -lws2_32
endif

# Common Includes
INCLUDES += -Ilib/gui -Icore/lang -I.

# Directory Structure
BUILD_ROOT = build
BIN_DIR = bin
BUILD_DIR = $(BUILD_ROOT)/$(OS_NAME)

# Source directories
GUI_DIR = lib/gui

# Source files
LANG_SRC = core/lang/lexer.cpp core/lang/parser.cpp core/lang/interpreter.cpp core/lang/value_impl.cpp
LIB_SRC = lib/register.cpp lib/string/string.cpp lib/array/array.cpp lib/map/map.cpp
GUI_SRC = lib/gui/renderer.cpp lib/gui/parser.cpp lib/gui/widgets.cpp lib/gui/layout.cpp lib/gui/minigui.cpp
MAIN_SRC = sunda.cpp

# Object files (placed in BUILD_DIR)
LANG_OBJ = $(patsubst core/lang/%.cpp, $(BUILD_DIR)/lang_%.o, $(LANG_SRC))
LIB_OBJ = $(BUILD_DIR)/register.o $(BUILD_DIR)/string_string.o $(BUILD_DIR)/array_array.o $(BUILD_DIR)/map_map.o
GUI_OBJ = $(patsubst lib/gui/%.cpp, $(BUILD_DIR)/gui_%.o, $(GUI_SRC))
MAIN_OBJ = $(BUILD_DIR)/sunda.o

OBJS = $(MAIN_OBJ) $(LIB_OBJ) $(LANG_OBJ) $(GUI_OBJ)

TARGET_NAME = sunda$(EXE_EXT)
TARGET = $(BUILD_DIR)/$(TARGET_NAME)
FINAL_BIN = $(BIN_DIR)/sunda$(EXE_EXT)

.PHONY: all clean check_deps setup copy sunda

all: check_deps setup $(TARGET) copy

# Check for required tools and libraries
check_deps:
	@echo "Checking dependencies for $(OS_NAME)..."
	@command -v $(CXX) >/dev/null 2>&1 || { echo >&2 "Error: $(CXX) is not installed."; exit 1; }
ifeq ($(OS_NAME),macos)
	@ls $(HOMEBREW_PREFIX)/lib/libglfw.dylib >/dev/null 2>&1 || { echo >&2 "Error: glfw not found in $(HOMEBREW_PREFIX)"; exit 1; }
	@ls $(HOMEBREW_PREFIX)/lib/libfreetype.dylib >/dev/null 2>&1 || { echo >&2 "Error: freetype not found in $(HOMEBREW_PREFIX)"; exit 1; }
else ifeq ($(OS_NAME),linux)
	@pkg-config --exists glfw3 || { echo >&2 "Error: glfw3 not found via pkg-config"; exit 1; }
	@pkg-config --exists freetype2 || { echo >&2 "Error: freetype2 not found via pkg-config"; exit 1; }
	@pkg-config --exists sqlite3 || { echo >&2 "Error: sqlite3 not found via pkg-config"; exit 1; }
	@pkg-config --exists mysqlclient || { echo >&2 "Error: mysqlclient not found via pkg-config"; exit 1; }
	@pkg-config --exists libcurl || { echo >&2 "Error: libcurl not found via pkg-config"; exit 1; }
	@pkg-config --exists openssl || { echo >&2 "Error: openssl not found via pkg-config"; exit 1; }
else ifeq ($(OS_NAME),windows)
	@test -f win/glfw3.dll || { echo >&2 "Error: glfw3.dll not found in win/"; exit 1; }
	@test -f win/freetype.dll || { echo >&2 "Error: freetype.dll not found in win/"; exit 1; }
	@test -f win/sqlite3.dll || { echo >&2 "Error: sqlite3.dll not found in win/"; exit 1; }
	@test -f win/mysqlcppconn-10-vs14.dll || { echo >&2 "Error: mysqlcppconn-10-vs14.dll not found in win/"; exit 1; }
	@test -f win/libcurl-x64.dll || { echo >&2 "Error: libcurl-x64.dll not found in win/"; exit 1; }
endif
	@echo "Dependencies OK."

setup:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# Build rules
$(BUILD_DIR)/gui_%.o: $(GUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/lang_%.o: core/lang/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/register.o: lib/register.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/string_string.o: lib/string/string.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/array_array.o: lib/array/array.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/map_map.o: lib/map/map.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/sunda.o: sunda.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OBJS) -o $(TARGET) $(LDFLAGS)

copy: $(TARGET)
	@cp $(TARGET) $(FINAL_BIN)
ifeq ($(OS_NAME),windows)
	@echo "Copying Windows DLLs to $(BIN_DIR)..."
	@cp win/*.dll $(BIN_DIR)/
endif
	@echo "Build successful! Binary location: $(FINAL_BIN)"

sunda: all

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# MinGW Cross-Compilation Target (macOS/Linux -> Windows)
mingw:
	@echo "Building for Windows using MinGW cross-compiler..."
	@mkdir -p build/windows
	@mkdir -p bin
	x86_64-w64-mingw32-g++ -std=c++17 -O2 -Wall \
		-Iwin/include -Ilib/gui -Icore/lang -I. \
		sunda.cpp \
		lib/register.cpp \
		lib/string/string.cpp \
		lib/array/array.cpp \
		lib/map/map.cpp \
		core/lang/lexer.cpp \
		core/lang/parser.cpp \
		core/lang/interpreter.cpp \
		core/lang/value_impl.cpp \
		lib/gui/renderer.cpp \
		lib/gui/parser.cpp \
		lib/gui/widgets.cpp \
		lib/gui/layout.cpp \
		lib/gui/minigui.cpp \
		-Lwin \
		-lglfw3 -lfreetype -lsqlite3 -lcurl -lmysqlcppconn-10-vs14 \
		-lssl -lcrypto -lgdi32 -lopengl32 -lwinmm -lws2_32 \
		-o build/windows/sunda.exe
	@cp build/windows/sunda.exe bin/sunda.exe
	@cp win/*.dll bin/
	@echo "Windows build complete! Binary: bin/sunda.exe"
	@echo "DLLs copied to bin/"

# MinGW Compile-Only (no linking) - for testing when .lib files are not available
mingw-compile-only:
	@echo "Compiling for Windows (compile-only, no linking)..."
	@mkdir -p build/windows
	x86_64-w64-mingw32-g++ -std=c++17 -O2 -Wall -c \
		-Iwin/include -Ilib/gui -Icore/lang -I. \
		sunda.cpp -o build/windows/sunda.o
	x86_64-w64-mingw32-g++ -std=c++17 -O2 -Wall -c \
		-Iwin/include -Ilib/gui -Icore/lang -I. \
		lib/register.cpp -o build/windows/register.o
	x86_64-w64-mingw32-g++ -std=c++17 -O2 -Wall -c \
		-Iwin/include -Ilib/gui -Icore/lang -I. \
		lib/string/string.cpp -o build/windows/string.o
	x86_64-w64-mingw32-g++ -std=c++17 -O2 -Wall -c \
		-Iwin/include -Ilib/gui -Icore/lang -I. \
		core/lang/interpreter.cpp -o build/windows/interpreter.o
	@echo "Compilation successful! Object files in build/windows/"
	@echo "Note: Linking skipped (requires .lib files in win/lib/)"

# MinGW Minimal Build - Links only with Windows system libraries (no external deps)
# This creates a basic executable but without GLFW/GUI, database, HTTP features
mingw-minimal:
	@echo "Building minimal Windows executable (system libs only)..."
	@mkdir -p build/windows
	@mkdir -p bin
	x86_64-w64-mingw32-g++ -std=c++17 -O2 -Wall \
		-DMINIMAL_BUILD \
		-Iwin/include -Ilib/gui -Icore/lang -I. \
		sunda.cpp \
		lib/register.cpp \
		lib/string/string.cpp \
		lib/array/array.cpp \
		lib/map/map.cpp \
		core/lang/lexer.cpp \
		core/lang/parser.cpp \
		core/lang/interpreter.cpp \
		core/lang/value_impl.cpp \
		-lgdi32 -lwinmm -lws2_32 \
		-o build/windows/sunda.exe
	@cp build/windows/sunda.exe bin/sunda.exe
	@echo "✅ Minimal Windows build complete! Binary: bin/sunda.exe"
	@echo "Note: GUI, Database, HTTP features disabled (no external libs)"
