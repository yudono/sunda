#ifndef SUNDA_JSON_LIB_H
#define SUNDA_JSON_LIB_H

#include "../../core/lang/interpreter.h"
#include <vector>
#include <string>
#include <map>
#include <cctype>

namespace JSONLib {

class JsonParser {
    std::string source;
    size_t pos = 0;

    void skipWhitespace() {
        while (pos < source.length() && std::isspace(source[pos])) pos++;
    }

    char peek() {
        if (pos >= source.length()) return '\0';
        return source[pos];
    }

    char advance() {
        if (pos >= source.length()) return '\0';
        return source[pos++];
    }

    Value parseValue() {
        skipWhitespace();
        char c = peek();
        if (c == '{') return parseObject();
        if (c == '[') return parseArray();
        if (c == '"') return parseString();
        if (std::isdigit(c) || c == '-') return parseNumber();
        
        if (source.substr(pos, 4) == "true") { pos += 4; return Value("true", 1, true); }
        if (source.substr(pos, 5) == "false") { pos += 5; return Value("false", 0, true); }
        if (source.substr(pos, 4) == "null") { pos += 4; return Value("null", 0, false); }
        
        return Value("undefined", 0, false);
    }

    Value parseObject() {
        advance(); // {
        std::map<std::string, Value> map;
        while (peek() != '}' && peek() != '\0') {
            skipWhitespace();
            if (peek() == '}') break; // handle empty object
            Value key = parseString();
            skipWhitespace();
            if (advance() != ':') break;
            map[key.strVal] = parseValue();
            skipWhitespace();
            if (peek() == ',') advance();
        }
        if (peek() == '}') advance(); 
        return Value(map);
    }

    Value parseArray() {
        advance(); // [
        std::vector<Value> list;
        while (peek() != ']' && peek() != '\0') {
            skipWhitespace();
            if (peek() == ']') break; // handle empty array
            list.push_back(parseValue());
            skipWhitespace();
            if (peek() == ',') advance();
            skipWhitespace();
        }
        if (peek() == ']') advance();
        return Value(list);
    }

    Value parseString() {
        if (peek() == '"') advance(); // consume start quote
        std::string s;
        while (peek() != '"' && peek() != '\0') {
            if (peek() == '\\') {
                advance();
                char esc = advance();
                if (esc == 'n') s += '\n';
                else if (esc == 't') s += '\t';
                else if (esc == 'r') s += '\r';
                else if (esc == '"') s += '"';
                else if (esc == '\\') s += '\\';
                else s += esc;
            } else {
                s += advance();
            }
        }
        if (peek() == '"') advance(); // consume end quote
        return Value(s, 0, false);
    }

    Value parseNumber() {
        size_t start = pos;
        if (peek() == '-') advance();
        while (std::isdigit(peek())) advance();
        if (peek() == '.') {
            advance();
            while (std::isdigit(peek())) advance();
        }
        int val = std::stoi(source.substr(start, pos - start));
        return Value("", val, true);
    }

public:
    JsonParser(std::string s) : source(s) {}
    Value parse() { return parseValue(); }
};

void register_json(Interpreter& interpreter) {
    interpreter.registerNative("json_parse", [](std::vector<Value> args) -> Value {
        if (args.empty()) return Value("undefined", 0, false);
        JsonParser parser(args[0].toString());
        return parser.parse();
    });
}

} // namespace JSONLib

#endif
