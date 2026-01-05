#ifndef ANIS_DEBUGGER_H
#define ANIS_DEBUGGER_H

#include <string>
#include <iostream>
#include <sstream>
#include <vector>

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_GRAY    "\033[0;37m"

class Debugger {
public:
    static bool isReplMode;
    static std::vector<std::string> callStack;
    
    // Stack trace management
    static void pushCall(const std::string& name, int line) {
        callStack.push_back(name + " (line " + std::to_string(line) + ")");
    }
    
    static void popCall() {
        if (!callStack.empty()) callStack.pop_back();
    }
    
    static void clearCallStack() {
        callStack.clear();
    }
    
    static std::string getStackTrace() {
        if (callStack.empty()) return "";
        
        std::string trace = "\n" + std::string(COLOR_GRAY) + "Call stack:" + std::string(COLOR_RESET) + "\n";
        int depth = 0;
        for (auto it = callStack.rbegin(); it != callStack.rend(); ++it) {
            trace += "  " + std::to_string(++depth) + ". at " + *it + "\n";
        }
        return trace;
    }

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
        
        // Show stack trace if available
        std::string stackTrace = getStackTrace();
        if (!stackTrace.empty()) {
            std::cerr << stackTrace;
        }
        
        if (!isReplMode) exit(1);
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
        if (!isReplMode) exit(1);
    }
    
    // Enhanced runtime error with better context
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

        // Show code context with surrounding lines
        if (line > 0 && !source.empty()) {
            showCodeContext(source, line);
        }
        
        // Show stack trace
        std::string stackTrace = getStackTrace();
        if (!stackTrace.empty()) {
            std::cerr << stackTrace;
        }
        
        if (!isReplMode) exit(1);
    }
    
    // Type-specific errors
    static void typeError(const std::string& message, int line = 0, const std::string& source = "", const std::string& file = "") {
        std::cerr << COLOR_MAGENTA << "❌ TYPE ERROR" << COLOR_RESET << std::endl;
        std::cerr << "   " << message << std::endl;
        
        if (!file.empty() || line > 0) {
            std::string location = file;
            if (line > 0) {
                if (!location.empty()) location += ":";
                location += std::to_string(line);
            }
            std::cerr << "   At " << location << ":" << std::endl;
        }
        
        if (line > 0 && !source.empty()) {
            showCodeContext(source, line);
        }
        
        std::string stackTrace = getStackTrace();
        if (!stackTrace.empty()) {
            std::cerr << stackTrace;
        }
        
        if (!isReplMode) exit(1);
    }
    
    static void referenceError(const std::string& message, int line = 0, const std::string& source = "", const std::string& file = "") {
        std::cerr << COLOR_RED << "❌ REFERENCE ERROR" << COLOR_RESET << std::endl;
        std::cerr << "   " << message << std::endl;
        
        if (!file.empty() || line > 0) {
            std::string location = file;
            if (line > 0) {
                if (!location.empty()) location += ":";
                location += std::to_string(line);
            }
            std::cerr << "   At " << location << ":" << std::endl;
        }
        
        if (line > 0 && !source.empty()) {
            showCodeContext(source, line);
        }
        
        std::string stackTrace = getStackTrace();
        if (!stackTrace.empty()) {
            std::cerr << stackTrace;
        }
        
        if (!isReplMode) exit(1);
    }
    
    static void rangeError(const std::string& message, int line = 0, const std::string& source = "", const std::string& file = "") {
        std::cerr << COLOR_YELLOW << "❌ RANGE ERROR" << COLOR_RESET << std::endl;
        std::cerr << "   " << message << std::endl;
        
        if (!file.empty() || line > 0) {
            std::string location = file;
            if (line > 0) {
                if (!location.empty()) location += ":";
                location += std::to_string(line);
            }
            std::cerr << "   At " << location << ":" << std::endl;
        }
        
        if (line > 0 && !source.empty()) {
            showCodeContext(source, line);
        }
        
        std::string stackTrace = getStackTrace();
        if (!stackTrace.empty()) {
            std::cerr << stackTrace;
        }
        
        if (!isReplMode) exit(1);
    }
    
private:
    // Show code context with 2 lines before and after
    static void showCodeContext(const std::string& source, int errorLine) {
        std::stringstream ss(source);
        std::string lineText;
        int current = 1;
        std::vector<std::pair<int, std::string>> lines;
        
        // Collect all lines
        while (std::getline(ss, lineText)) {
            lines.push_back({current, lineText});
            current++;
        }
        
        // Show context: 2 lines before, error line, 2 lines after
        int startLine = std::max(1, errorLine - 2);
        int endLine = std::min((int)lines.size(), errorLine + 2);
        
        std::cerr << std::endl;
        for (int i = startLine; i <= endLine; i++) {
            if (i > 0 && i <= (int)lines.size()) {
                bool isErrorLine = (i == errorLine);
                std::string lineNum = std::to_string(i);
                
                // Pad line number
                while (lineNum.length() < 4) lineNum = " " + lineNum;
                
                if (isErrorLine) {
                    std::cerr << COLOR_RED << "  > " << lineNum << " | " << lines[i-1].second << COLOR_RESET << std::endl;
                    // Add pointer
                    std::cerr << COLOR_RED << "         | ^" << COLOR_RESET << std::endl;
                } else {
                    std::cerr << COLOR_GRAY << "    " << lineNum << " | " << lines[i-1].second << COLOR_RESET << std::endl;
                }
            }
        }
        std::cerr << std::endl;
    }
};

// Static member definitions
inline bool Debugger::isReplMode = false;
inline std::vector<std::string> Debugger::callStack;

#endif
