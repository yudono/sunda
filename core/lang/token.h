#ifndef SUNDA_TOKEN_H
#define SUNDA_TOKEN_H

#include <string>
#include <iostream>

enum TokenType {
    TOK_EOF,
    TOK_VAR, TOK_IF, TOK_FOR, TOK_WHILE, TOK_FUNCTION, TOK_IMPORT, TOK_FROM, TOK_RETURN, TOK_EXPORT,
    TOK_SWITCH, TOK_CASE, TOK_DEFAULT, TOK_ELSE, // Added TOK_ELSE
    TOK_CONST, TOK_LBRACKET, TOK_RBRACKET,
    TOK_IDENTIFIER, TOK_STRING, TOK_NUMBER,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
    TOK_EQ, TOK_EQEQ, TOK_NE, TOK_SEMICOLON, TOK_COLON, TOK_COMMA, TOK_DOT, TOK_QUESTION,
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PLUS_EQUAL,
    TOK_LT, TOK_GT, TOK_LTE, TOK_GTE,
    TOK_AND, TOK_OR, TOK_DOT_DOT_DOT,
    TOK_ARROW, // =>
    TOK_BANG   // !
};

struct Token {
    TokenType type;
    std::string text;
    int line = 0;
    
    Token(TokenType t, std::string txt, int l = 0) : type(t), text(txt), line(l) {}
    Token() : type(TOK_EOF), text(""), line(0) {}
};

#endif
