#ifndef SUNDA_LEXER_H
#define SUNDA_LEXER_H

#include <string>
#include <vector>
#include "token.h"

class Lexer {
    std::string src;
    size_t pos = 0;
    int line = 1;
public:
    Lexer(const std::string& source) : src(source) {}
    std::vector<Token> tokenize();
private:
    char peek();
    char advance();
    bool match(char expected);
    void skipWhitespace();
};

#endif
