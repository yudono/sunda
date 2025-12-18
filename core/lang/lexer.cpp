#include "lexer.h"
#include <cctype>
#include <iostream>

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    auto addToken = [&](TokenType t, std::string text) {
        tokens.push_back({t, text, line});
    };

    while (pos < src.size()) {
        char c = peek();
        if (c == '\n') {
            line++;
            advance();
        } else if (isspace(c)) {
            advance();
        } else if (isalpha(c) || c == '_') {
            std::string ident;
            while (isalnum(peek()) || peek() == '_') ident += advance();
            
            if (ident == "var") addToken(TOK_VAR, ident);
            else if (ident == "if") addToken(TOK_IF, ident);
            else if (ident == "for") addToken(TOK_FOR, ident);
            else if (ident == "while") addToken(TOK_WHILE, ident);
            else if (ident == "function") addToken(TOK_FUNCTION, ident);
            else if (ident == "import") addToken(TOK_IMPORT, ident);
            else if (ident == "from") addToken(TOK_FROM, ident);
            else if (ident == "return") addToken(TOK_RETURN, ident);
            else if (ident == "export") addToken(TOK_EXPORT, ident);
            else if (ident == "switch") addToken(TOK_SWITCH, ident);
            else if (ident == "case") addToken(TOK_CASE, ident);
            else if (ident == "default") addToken(TOK_DEFAULT, ident);
            else if (ident == "const") addToken(TOK_CONST, ident);
            else if (ident == "else") addToken(TOK_ELSE, ident);
            else if (ident == "class") addToken(TOK_CLASS, ident);
            else if (ident == "new") addToken(TOK_NEW, ident);
            else if (ident == "extends") addToken(TOK_EXTENDS, ident);
            else if (ident == "super") addToken(TOK_SUPER, ident);
            else if (ident == "static") addToken(TOK_STATIC, ident);
            else if (ident == "this") addToken(TOK_THIS, ident);
            else if (ident == "get") addToken(TOK_GET, ident);
            else if (ident == "set") addToken(TOK_SET, ident);
            else addToken(TOK_IDENTIFIER, ident);
        } else if (isdigit(c)) {
            // ...
            std::string num;
            while (isdigit(peek())) num += advance();
            // check for float
            if (peek() == '.' && pos + 1 < src.size() && isdigit(src[pos+1])) {
                 num += advance(); // .
                 while (isdigit(peek())) num += advance();
            }
            addToken(TOK_NUMBER, num);
        } else if (c == '"') {
             // ...
             advance(); // skip opening
            std::string str;
            while (peek() != '"' && pos < src.size()) {
                 str += advance();
            }
            advance(); // skip closing
            addToken(TOK_STRING, str);
        } else if (c == '`') {
             // ...
             advance(); // skip opening
            std::string str;
            while (peek() != '`' && pos < src.size()) {
                 str += advance();
            }
            advance(); // skip closing
            addToken(TOK_STRING, str);
        } else if (c == '\'') {
             // ...
             advance(); // skip opening
            std::string str;
            while (peek() != '\'' && pos < src.size()) {
                 str += advance();
            }
            advance(); // skip closing
            addToken(TOK_STRING, str);
        } else {
            advance();
            if (c == '(') addToken(TOK_LPAREN, "(");
            else if (c == ')') addToken(TOK_RPAREN, ")");
            else if (c == '{') {
                 if (peek() == '*') {
                      // Multi-line comment {* ... *}
                      advance(); // skip *
                      while (pos + 1 < src.size()) {
                           if (src[pos] == '*' && src[pos+1] == '}') {
                                pos += 2;
                                break;
                           }
                           if (src[pos] == '\n') line++; // Track newlines in comments
                           pos++;
                      }
                 } else {
                      addToken(TOK_LBRACE, "{");
                 }
            }
            else if (c == '}') addToken(TOK_RBRACE, "}");
            else if (c == '[') addToken(TOK_LBRACKET, "[");
            else if (c == ']') addToken(TOK_RBRACKET, "]");
            else if (c == ';') addToken(TOK_SEMICOLON, ";");
            else if (c == ':') addToken(TOK_COLON, ":");
            else if (c == '?') addToken(TOK_QUESTION, "?");
            else if (c == ',') addToken(TOK_COMMA, ",");
            else if (c == '.') {
                 if (peek() == '.' && pos + 1 < src.size() && src[pos+1] == '.') {
                      advance(); advance(); // consume two more dots
                      addToken(TOK_DOT_DOT_DOT, "...");
                 } else {
                      addToken(TOK_DOT, ".");
                 }
            }
            else if (c == '#') {
                 // Private identifier #field
                 // advance() already consumed '#'
                 std::string ident = "#";
                 while (isalnum(peek()) || peek() == '_') ident += advance();
                 addToken(TOK_PRIVATE_IDENTIFIER, ident);
            }
            else if (c == '+') {
                 if (peek() == '=') { advance(); addToken(TOK_PLUS_EQUAL, "+="); }
                 else addToken(TOK_PLUS, "+");
            }
            else if (c == '-') {
                if (peek() == '>') { advance(); addToken(TOK_ARROW, "->"); } // Just in case users type ->
                else addToken(TOK_MINUS, "-");
            }
            else if (c == '&') {
                 if (peek() == '&') { advance(); addToken(TOK_AND, "&&"); }
            }
            else if (c == '|') {
                 if (peek() == '|') { advance(); addToken(TOK_OR, "||"); }
            }
            else if (c == '*') addToken(TOK_STAR, "*");
            else if (c == '/') {
                 if (peek() == '/') {
                      // Comment //
                      while (peek() != '\n' && pos < src.size()) advance();
                 } 
                 else if (peek() == '*') {
                      // Comment /* ... */
                      advance(); // skip *
                      while (pos + 1 < src.size()) {
                           if (src[pos] == '*' && src[pos+1] == '/') {
                                pos += 2;
                                break;
                           }
                           if (src[pos] == '\n') line++; // Track newlines
                           pos++;
                      }
                 }
                 else {
                      addToken(TOK_SLASH, "/");
                 }
            }
            else if (c == '<') {
                 if (peek() == '=') { advance(); addToken(TOK_LTE, "<="); }
                 else addToken(TOK_LT, "<");
            }
            else if (c == '>') {
                 if (peek() == '=') { advance(); addToken(TOK_GTE, ">="); }
                 else addToken(TOK_GT, ">");
            }
            else if (c == '=') {
                if (peek() == '=') { advance(); addToken(TOK_EQEQ, "=="); }
                else if (peek() == '>') { advance(); addToken(TOK_ARROW, "=>"); }
                else addToken(TOK_EQ, "=");
            }
            else if (c == '!') {
                 if (peek() == '=') { advance(); addToken(TOK_NE, "!="); }
                 else addToken(TOK_BANG, "!");
            }
        }
    }
    tokens.push_back({TOK_EOF, "", line});
    return tokens;
}

char Lexer::peek() {
    if (pos >= src.size()) return 0;
    return src[pos];
}
char Lexer::advance() {
    if (pos >= src.size()) return 0;
    return src[pos++];
}
