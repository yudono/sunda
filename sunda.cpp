#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
// Lang
#include "core/lang/lexer.h"
#include "core/lang/parser.h"
#include "core/lang/interpreter.h"
#include "core/debugger.h"
#include "lib/gui/minigui.h"
#include "lib/gui/layout.h"
#include "lib/register.h"
#include <curl/curl.h>
#include <curl/curl.h>
#include <csignal>
#include <atomic>
#ifdef __linux__
#include <execinfo.h>
#endif
#include <unistd.h>

// crash handler
void crash_handler(int sig) {
#ifdef __linux__
  void *array[10];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, 10);

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
#else
  fprintf(stderr, "Error: signal %d (backtrace not available)\n", sig);
  exit(1);
#endif
}
#include <atomic>

// Global interrupt flag
std::atomic<bool> g_interrupt(false);

void signal_handler(int signum) {
    if (signum == SIGINT) {
        g_interrupt = true;
    }
}

// Define global appState needed by minigui
AppState appState;

// Define static member of Debugger
bool Debugger::isReplMode = false;

void printHelp() {
    std::cout << COLOR_CYAN << "Sunda Programming Language" << COLOR_RESET << std::endl;
    std::cout << std::endl;
    std::cout << COLOR_GREEN << "USAGE:" << COLOR_RESET << std::endl;
    std::cout << "  sunda                        Enter REPL mode" << std::endl;
    std::cout << "  sunda <file.sd>              Run a Sunda script" << std::endl;
    std::cout << "  sunda --help                 Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << COLOR_GREEN << "EXAMPLES:" << COLOR_RESET << std::endl;
    std::cout << "  sunda                        Start interactive shell" << std::endl;
    std::cout << "  sunda examples/hello.sd      Run hello.sd" << std::endl;
    std::cout << "  sunda myapp/main.sd          Run GUI application" << std::endl;
    std::cout << std::endl;
    std::cout << COLOR_GREEN << "FEATURES:" << COLOR_RESET << std::endl;
    std::cout << "  • Modern JavaScript-like syntax" << std::endl;
    std::cout << "  • Arrow functions: (a, b) => { ... }" << std::endl;
    std::cout << "  • Object spread: { ...obj, key: value }" << std::endl;
    std::cout << "  • Ternary operator: condition ? true : false" << std::endl;
    std::cout << "  • JSX-like GUI components" << std::endl;
    std::cout << "  • Built-in GUI library" << std::endl;
    std::cout << std::endl;
}

void runREPL() {
    Debugger::isReplMode = true;
    Interpreter interpreter;
    register_std_libs(interpreter);

    std::cout << COLOR_CYAN << "Sunda REPL (v1.0.0)" << COLOR_RESET << std::endl;
    std::cout << "Type 'exit' to quit." << std::endl;

    std::string line;
    while (true) {
        std::cout << COLOR_GREEN << "sunda> " << COLOR_RESET;
        if (!std::getline(std::cin, line) || line == "exit") break;
        if (line.empty()) continue;

        try {
            // 1. Lex
            Lexer lexer(line);
            std::vector<Token> tokens = lexer.tokenize();

            // 2. Parse
            Parser parser(tokens);
            std::vector<std::shared_ptr<Stmt>> statements = parser.parse();

            // 3. Interpret
            interpreter.sourceCode = line;
            interpreter.hasLastExpressionValue = false; // Reset before run
            interpreter.interpret(statements);
            
            if (interpreter.hasLastExpressionValue) {
                std::cout << COLOR_BLUE << "=> " << COLOR_RESET << interpreter.lastExpressionValue.toString() << std::endl;
            }
        } catch (const std::exception& e) {
            // Error already printed by Debugger if it didn't exit
        } catch (...) {
            std::cerr << "Unknown error occurred" << std::endl;
        }
    }
}
// Global base path for relative resource loading (defined in layout.cpp)
extern std::string g_basePath;

int runFile(std::string filePath, bool dumpTokens) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << filePath << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // Set global base path
    size_t lastSlash = filePath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        g_basePath = filePath.substr(0, lastSlash + 1);
    }

    try {
        // 1. Lex
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();
        
        if (dumpTokens) {
            std::cout << "TOKENS:" << std::endl;
            for (const auto& t : tokens) {
                std::cout << "Line " << t.line << ": " << t.type << " '" << t.text << "'" << std::endl;
            }
            return 0;
        }

        // 2. Parse
        Parser parser(tokens);
        std::vector<std::shared_ptr<Stmt>> statements = parser.parse();

        // 3. Interpret
        Interpreter interpreter;
        interpreter.sourceCode = source;
        interpreter.currentFile = filePath;
        
        register_std_libs(interpreter);
        interpreter.interpret(statements);
    } catch (...) {
        // Errors handled by Debugger
        return 1;
    }

    return 0;
}

int main(int argc, char* argv[]) {
    signal(SIGSEGV, crash_handler);
    signal(SIGINT, signal_handler);
    curl_global_init(CURL_GLOBAL_ALL);

    int result = 0;
    if (argc < 2) {
        runREPL();
    } else {
        bool dumpTokens = false;
        std::string filePath;
        
        if (std::string(argv[1]) == "--dump-tokens") {
            dumpTokens = true;
            if (argc > 2) filePath = argv[2];
        } else {
            filePath = argv[1];
            if (argc > 2 && std::string(argv[2]) == "--dump-tokens") dumpTokens = true;
        }

        if (filePath.empty()) {
            printHelp();
        } else {
            result = runFile(filePath, dumpTokens);
        }
    }

    curl_global_cleanup();
    return result;
}
