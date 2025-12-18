#ifndef SUNDA_DEBUGGER_H
#define SUNDA_DEBUGGER_H

#include <string>
#include <iostream>
#include <sstream>

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_CYAN    "\033[1;36m"

class Debugger {
public:
    static void error(const std::string& message, const std::string& file = "", int line = 0) {
        std::cerr << COLOR_RED << "❌ ERROR";
        if (!file.empty()) {
            std::cerr << " in " << file;
            if (line > 0) {
                std::cerr << ":" << line;
            }
        }
        std::cerr << COLOR_RESET << std::endl;
        std::cerr << COLOR_RED << "   " << message << COLOR_RESET << std::endl;
        exit(1);
    }
    
    static void warning(const std::string& message, const std::string& file = "", int line = 0) {
        std::cerr << COLOR_YELLOW << "⚠️  WARNING";
        if (!file.empty()) {
            std::cerr << " in " << file;
            if (line > 0) {
                std::cerr << ":" << line;
            }
        }
        std::cerr << COLOR_RESET << std::endl;
        std::cerr << COLOR_YELLOW << "   " << message << COLOR_RESET << std::endl;
    }
    
    static void info(const std::string& message) {
        std::cerr << COLOR_CYAN << "ℹ️  " << message << COLOR_RESET << std::endl;
    }
    
    static void success(const std::string& message) {
        std::cerr << COLOR_GREEN << "✓ " << message << COLOR_RESET << std::endl;
    }
    
    static void parseError(const std::string& message, const std::string& token, int line = 0) {
        std::cerr << COLOR_RED << "❌ PARSE ERROR";
        if (line > 0) {
            std::cerr << " at line " << line;
        }
        std::cerr << COLOR_RESET << std::endl;
        std::cerr << COLOR_RED << "   " << message << COLOR_RESET << std::endl;
        if (!token.empty()) {
            std::cerr << COLOR_YELLOW << "   Near: '" << token << "'" << COLOR_RESET << std::endl;
        }
        exit(1);
    }
    
    static void runtimeError(const std::string& message, int line = 0, const std::string& source = "", const std::string& file = "") {
        std::cerr << COLOR_RED << "❌ RUNTIME ERROR" << COLOR_RESET << std::endl;
        std::cerr << "   " << message << std::endl;
        
        std::string location = "";
        if (!file.empty()) location += file;
        if (line > 0) {
            if (!location.empty()) location += ":";
            location += std::to_string(line);
        }
        
        if (!location.empty()) {
            std::cerr << "   At " << location << ":" << std::endl;
        }

        if (line > 0 && !source.empty()) {
             std::stringstream ss(source);
             std::string lineText;
             int current = 1;
             while (std::getline(ss, lineText)) {
                 if (current == line) {
                     std::cerr << "     " << lineText << std::endl;
                     std::cerr << "     " << COLOR_RED << "^" << COLOR_RESET << std::endl;
                     break;
                 }
                 current++;
             }
        }
        exit(1);
    }
};

#endif
