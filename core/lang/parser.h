#ifndef SUNDA_PARSER_H
#define SUNDA_PARSER_H
#define SUNDA_PARSER_H

#include "token.h"
#include <vector>
#include <string>
#include <memory>
#include <map>
#include "lexer.h"

// AST Base
struct Stmt { 
    virtual ~Stmt() = default; 
    int line = 0;
};
struct Expr { 
    virtual ~Expr() = default; 
    int line = 0;
};

// Expressions
struct LiteralExpr : Expr {
    std::string value; // Store as string, interpret later
    bool isString;
    LiteralExpr(std::string v, bool isStr) : value(v), isString(isStr) {}
};

struct VarExpr : Expr {
    std::string name;
    VarExpr(std::string n) : name(n) {}
};

struct CallExpr : Expr {
    std::shared_ptr<Expr> callee; // Changed from string to Expr
    std::vector<std::shared_ptr<Expr>> args;
    CallExpr(std::shared_ptr<Expr> c, std::vector<std::shared_ptr<Expr>> a) : callee(c), args(a) {}
};

struct MemberExpr : Expr {
    std::shared_ptr<Expr> object;
    std::shared_ptr<Expr> property;
    bool computed; // true for [], false for .
    MemberExpr(std::shared_ptr<Expr> o, std::shared_ptr<Expr> p, bool c) : object(o), property(p), computed(c) {}
};

struct ObjectExpr : Expr {
    std::map<std::string, std::shared_ptr<Expr>> properties;
    ObjectExpr(std::map<std::string, std::shared_ptr<Expr>> p) : properties(p) {}
};

struct ArrayExpr : Expr {
    std::vector<std::shared_ptr<Expr>> elements;
    ArrayExpr(std::vector<std::shared_ptr<Expr>> e) : elements(e) {}
};

struct SpreadExpr : Expr {
    std::shared_ptr<Expr> argument;
    SpreadExpr(std::shared_ptr<Expr> arg) : argument(arg) {}
};

struct UnaryExpr : Expr {
    std::string op;
    std::shared_ptr<Expr> right;
    UnaryExpr(std::string o, std::shared_ptr<Expr> r) : op(o), right(r) {}
};

struct BinaryExpr : Expr {
    std::shared_ptr<Expr> left;
    std::string op;
    std::shared_ptr<Expr> right;
    BinaryExpr(std::shared_ptr<Expr> l, std::string o, std::shared_ptr<Expr> r) : left(l), op(o), right(r) {}
};

struct TernaryExpr : Expr {
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Expr> trueExpr;
    std::shared_ptr<Expr> falseExpr;
    TernaryExpr(std::shared_ptr<Expr> c, std::shared_ptr<Expr> t, std::shared_ptr<Expr> f) 
        : condition(c), trueExpr(t), falseExpr(f) {}
};

// Statements
struct BlockStmt : Stmt {
    std::vector<std::shared_ptr<Stmt>> statements;
};

struct VarDeclStmt : Stmt {
    std::string name;
    std::shared_ptr<Expr> initializer;
    VarDeclStmt(std::string n, std::shared_ptr<Expr> i) : name(n), initializer(i) {}
};

struct IfStmt : Stmt {
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Stmt> thenBranch;
    std::shared_ptr<Stmt> elseBranch;
    IfStmt(std::shared_ptr<Expr> c, std::shared_ptr<Stmt> t, std::shared_ptr<Stmt> e = nullptr) 
        : condition(c), thenBranch(t), elseBranch(e) {}
};

struct WhileStmt : Stmt {
    std::shared_ptr<Expr> condition;
    std::shared_ptr<Stmt> body;
    WhileStmt(std::shared_ptr<Expr> c, std::shared_ptr<Stmt> b) : condition(c), body(b) {}
};

struct Case {
    std::shared_ptr<Expr> value; // nullptr for default
    std::shared_ptr<Stmt> body; // usually BlockStmt
};

struct SwitchStmt : Stmt {
    std::shared_ptr<Expr> condition;
    std::vector<Case> cases;
    SwitchStmt(std::shared_ptr<Expr> c, std::vector<Case> cs) : condition(c), cases(cs) {}
};

struct FuncDeclStmt : Stmt {
    std::string name;
    std::vector<std::string> params; // Added params
    std::shared_ptr<BlockStmt> body;
    FuncDeclStmt(std::string n, std::vector<std::string> p, std::shared_ptr<BlockStmt> b) : name(n), params(p), body(b) {}
    FuncDeclStmt(std::string n, std::shared_ptr<BlockStmt> b) : name(n), body(b) {} // Legacy
};

struct ReturnStmt : Stmt {
     std::shared_ptr<Expr> value;
     ReturnStmt(std::shared_ptr<Expr> v) : value(v) {}
};

struct JsxExpr : Expr {
    std::string tagName; // Identifier
    std::map<std::string, std::shared_ptr<Expr>> attributes;
    std::vector<std::shared_ptr<Expr>> children;
    JsxExpr(std::string t, std::map<std::string, std::shared_ptr<Expr>> a, std::vector<std::shared_ptr<Expr>> c)
      : tagName(t), attributes(a), children(c) {}
};

struct FunctionExpr : Expr {
    std::vector<std::string> params; // Added params
    std::shared_ptr<BlockStmt> body;
    FunctionExpr(std::vector<std::string> p, std::shared_ptr<BlockStmt> b) : params(p), body(b) {}
    FunctionExpr(std::shared_ptr<BlockStmt> b) : body(b) {} // Legacy
};

// Imports
struct ImportStmt : Stmt {
    std::string moduleName; // "app" or "gui"
    std::vector<std::string> symbols; // for { x, y }
    // If symbols is empty, it's import "mod" (run whole thing)
    ImportStmt(std::string m, std::vector<std::string> s = {}) : moduleName(m), symbols(s) {}
};

struct DestructureStmt : Stmt {
    std::vector<std::string> names;
    std::shared_ptr<Expr> initializer;
    DestructureStmt(std::vector<std::string> n, std::shared_ptr<Expr> i) : names(n), initializer(i) {}
};

struct ExportStmt : Stmt {
    std::shared_ptr<Stmt> declaration;
    ExportStmt(std::shared_ptr<Stmt> d) : declaration(d) {}
};

struct ExprStmt : Stmt {
    std::shared_ptr<Expr> expr;
    ExprStmt(std::shared_ptr<Expr> e) : expr(e) {}
};

class Parser {
    std::vector<Token> tokens;
    size_t current = 0;
public:
    Parser(const std::vector<Token>& t) : tokens(t) {}
    std::vector<std::shared_ptr<Stmt>> parse();
private:
    std::shared_ptr<Stmt> declaration();
    std::shared_ptr<Stmt> statement();
    std::shared_ptr<Expr> expression();
    std::shared_ptr<Expr> assignment();
    std::shared_ptr<Expr> logicalOr();
    std::shared_ptr<Expr> logicalAnd();
    std::shared_ptr<Expr> equality();
    std::shared_ptr<Expr> comparison();
    std::shared_ptr<Expr> term();
    std::shared_ptr<Expr> factor();
    std::shared_ptr<Expr> unary();
    std::shared_ptr<Expr> call(); // Added
    std::shared_ptr<Expr> primary();
    
    bool match(TokenType t);
    Token consume(TokenType t, std::string err);
    std::shared_ptr<CallExpr> finishCall(std::string name);
    Token peek();
    Token peekNext();
    bool isAtEnd();
    
    // Missing helpers
    bool check(TokenType t);
    Token advance();
    Token previous();
};

#endif
