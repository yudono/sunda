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
    INCLUDES = -I$(HOMEBREW_PREFIX)/include -I$(HOMEBREW_PREFIX)/include/freetype2 -I$(HOMEBREW_PREFIX)/include/mysql
    LDFLAGS = -L$(HOMEBREW_PREFIX)/lib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lglfw -lfreetype -lsqlite3 -lmysqlclient -lcurl
else ifeq ($(OS_NAME),linux)
    INCLUDES = $(shell pkg-config --cflags glfw3 freetype2 sqlite3 mysqlclient libcurl)
    LDFLAGS = $(shell pkg-config --libs glfw3 freetype2 sqlite3 mysqlclient libcurl) -lGL -lX11 -lpthread -ldl
else ifeq ($(OS_NAME),windows)
    # Assuming MinGW/clang on Windows with libraries in standard paths
    INCLUDES = -I/usr/include/freetype2 -I/usr/include/mysql
    LDFLAGS = -lglfw3 -lfreetype -lsqlite3 -lmysqlclient -lcurl -lgdi32 -lopengl32 -lwinmm
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
	@echo "Build successful! Binary location: $(FINAL_BIN)"

sunda: all

clean:
	rm -rf $(BUILD_ROOT) $(BIN_DIR)
