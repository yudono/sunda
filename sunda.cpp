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

// Define global appState needed by minigui
AppState appState;

void printHelp() {
    std::cout << COLOR_CYAN << "Sunda Programming Language" << COLOR_RESET << std::endl;
    std::cout << std::endl;
    std::cout << COLOR_GREEN << "USAGE:" << COLOR_RESET << std::endl;
    std::cout << "  sunda <file.sd>              Run a Sunda script" << std::endl;
    std::cout << "  sunda --help                 Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << COLOR_GREEN << "EXAMPLES:" << COLOR_RESET << std::endl;
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        Debugger::error("No input file specified", "", 0);
        std::cerr << "Run 'sunda --help' for usage information" << std::endl;
        return 1;
    }
    
    std::string arg = argv[1];
    if (arg == "--help" || arg == "-h") {
        printHelp();
        return 0;
    }
    
    // Check flags
    bool dumpTokens = false;
    std::string filePath = arg;
    
    if (argc > 2) {
        std::string flag = argv[1];
        if (flag == "--dump-tokens") {
            dumpTokens = true;
            filePath = argv[2];
        } else {
             filePath = argv[1];
             if (std::string(argv[2]) == "--dump-tokens") dumpTokens = true;
        }
    }
    
    // Set global base path for relative resource loading
    std::string scriptPath = filePath;
    size_t lastSlash = scriptPath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        g_basePath = scriptPath.substr(0, lastSlash + 1);  // Include the slash
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Could not open file: " << argv[1] << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // 1. Lex
    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();
    
    if (dumpTokens) {
        std::cout << "TOKENS:" << std::endl;
        for (const auto& t : tokens) {
            std::cout << "Line " << t.line << ": " << t.type << " '" << t.text << "'" << std::endl;
        }
        return 0; // Exit after dump
    }

    // 2. Parse
    Parser parser(tokens);
    std::vector<std::shared_ptr<Stmt>> statements = parser.parse();

    // 3. Interpret
    Interpreter interpreter;
    interpreter.sourceCode = source; // Pass source for debugging
    
    // Register Libs
    register_std_libs(interpreter);

    interpreter.interpret(statements);

    return 0;
}
