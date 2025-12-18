#include "parser.h"
#include <iostream>
#include "../debugger.h"

std::vector<std::shared_ptr<Stmt>> Parser::parse() {
    std::vector<std::shared_ptr<Stmt>> statements;
    while (!isAtEnd()) {
        if (match(TOK_SEMICOLON)) continue; // Skip empty statements
        try {
            statements.push_back(declaration());
        } catch (...) {
            advance(); // synchronize
        }
    }
    return statements;
}

std::shared_ptr<Stmt> Parser::declaration() {
    // Skip extra semicolons
    while (match(TOK_SEMICOLON));
    
    // Export statement
    if (match(TOK_EXPORT)) {
        auto decl = declaration();  // Parse what follows (function, var, etc)
        return std::make_shared<ExportStmt>(decl);
    }
    
    if (match(TOK_VAR)) {
        Token name = consume(TOK_IDENTIFIER, "Expect variable name.");
        std::shared_ptr<Expr> init = nullptr;
        if (match(TOK_EQ)) {
            init = expression();
        }
        return std::make_shared<VarDeclStmt>(name.text, init);
    }
    // const handling (treat as var or destructuring)
    if (match(TOK_CONST)) {
        if (match(TOK_LBRACKET)) {
            // Destructuring: const [a, b] = ...
            std::vector<std::string> names;
            if (!check(TOK_RBRACKET)) {
                do {
                    Token t = consume(TOK_IDENTIFIER, "Expect variable name in destructuring.");
                    names.push_back(t.text);
                } while (match(TOK_COMMA));
            }
            consume(TOK_RBRACKET, "Expect ']' after destructuring list.");
            consume(TOK_EQ, "Expect '=' after destructuring declaration.");
            std::shared_ptr<Expr> init = expression();
            return std::make_shared<DestructureStmt>(names, init);
        } else {
             // Normal const var
             Token name = consume(TOK_IDENTIFIER, "Expect variable name.");
             std::shared_ptr<Expr> init = nullptr;
             if (match(TOK_EQ)) {
                 init = expression();
             }
             return std::make_shared<VarDeclStmt>(name.text, init);
        }
    }
    if (match(TOK_FUNCTION)) {
        // Named function declaration: function Name(a, b) { ... }
        Token name = consume(TOK_IDENTIFIER, "Expect function name.");
        consume(TOK_LPAREN, "Expect '(' after function name.");
        std::vector<std::string> params;
        if (!check(TOK_RPAREN)) {
            do {
                // Check for destructuring pattern { a, b, c }
                if (match(TOK_LBRACE)) {
                    std::vector<std::string> props;
                    if (!check(TOK_RBRACE)) {
                        do {
                            Token prop = consume(TOK_IDENTIFIER, "Expect property name in destructuring.");
                            props.push_back(prop.text);
                        } while (match(TOK_COMMA));
                    }
                    consume(TOK_RBRACE, "Expect '}' after destructuring pattern.");
                    // Store as special format: "__destruct:{a,b,c}"
                    std::string pattern = "__destruct:{";
                    for (size_t i = 0; i < props.size(); i++) {
                        pattern += props[i];
                        if (i < props.size() - 1) pattern += ",";
                    }
                    pattern += "}";
                    params.push_back(pattern);
                } else if (match(TOK_IDENTIFIER)) {
                    params.push_back(previous().text);
                }
            } while (match(TOK_COMMA));
        }
        consume(TOK_RPAREN, "Expect ')' after parens.");
        consume(TOK_LBRACE, "Expect '{' before function body.");
        std::shared_ptr<BlockStmt> body = std::make_shared<BlockStmt>();
        while (!check(TOK_RBRACE) && !isAtEnd()) {
            body->statements.push_back(declaration());
        }
        consume(TOK_RBRACE, "Expect '}' after body.");
        return std::make_shared<FuncDeclStmt>(name.text, params, body);
    }
    if (match(TOK_IMPORT)) {
        if (match(TOK_STRING)) {
            std::string mod = previous().text;
            return std::make_shared<ImportStmt>(mod);
        }
        if (match(TOK_LBRACE)) {
            std::vector<std::string> syms;
            if (!check(TOK_RBRACE)) {
                do {
                    if (match(TOK_IDENTIFIER)) syms.push_back(previous().text);
                } while (match(TOK_COMMA));
            }
            consume(TOK_RBRACE, "Expect '}' after import list.");
            if (match(TOK_FROM) || (check(TOK_IDENTIFIER) && peek().text == "from" && advance().type == TOK_IDENTIFIER)) {
                 // matched from
            }
            Token modToken = consume(TOK_STRING, "Expect module string after 'from'.");
            return std::make_shared<ImportStmt>(modToken.text, syms);
        }
    }

    return statement();
}

std::shared_ptr<Stmt> Parser::statement() {
    if (match(TOK_RETURN)) {
        std::shared_ptr<Expr> value = nullptr;
        if (!check(TOK_SEMICOLON) && !check(TOK_RBRACE)) { // rudimentary check
            value = expression();
        }
        // consume semicolon if present
        match(TOK_SEMICOLON);
        return std::make_shared<ReturnStmt>(value);
    }
    if (match(TOK_IF)) {
        consume(TOK_LPAREN, "Expect '(' after if.");
        std::shared_ptr<Expr> condition = expression();
        consume(TOK_RPAREN, "Expect ')' after condition.");
        std::shared_ptr<Stmt> thenBranch = statement();
        std::shared_ptr<Stmt> elseBranch = nullptr;
        if (match(TOK_ELSE)) {
            elseBranch = statement();
        }
        return std::make_shared<IfStmt>(condition, thenBranch, elseBranch);
    }
    if (match(TOK_WHILE)) {
        consume(TOK_LPAREN, "Expect '(' after while.");
        std::shared_ptr<Expr> condition = expression();
        consume(TOK_RPAREN, "Expect ')' after condition.");
        std::shared_ptr<Stmt> body = statement();
        return std::make_shared<WhileStmt>(condition, body);
    }
    if (match(TOK_SWITCH)) {
        consume(TOK_LPAREN, "Expect '(' after switch.");
        std::shared_ptr<Expr> condition = expression();
        consume(TOK_RPAREN, "Expect ')' after condition.");
        consume(TOK_LBRACE, "Expect '{' before switch cases.");
        
        std::vector<Case> cases;
        while (!check(TOK_RBRACE) && !isAtEnd()) {
            if (match(TOK_CASE)) {
                 std::shared_ptr<Expr> value = expression();
                 consume(TOK_COLON, "Expect ':' after case value.");
                 // Parse statements until next case/default/brace?
                 // Simplification: Require block or single statement
                 // Or loop declarations until case/default/rbrace
                 std::shared_ptr<BlockStmt> block = std::make_shared<BlockStmt>();
                 while (!check(TOK_CASE) && !check(TOK_DEFAULT) && !check(TOK_RBRACE) && !isAtEnd()) {
                     block->statements.push_back(declaration());
                 }
                 cases.push_back({value, block});
            } else if (match(TOK_DEFAULT)) {
                 consume(TOK_COLON, "Expect ':' after default.");
                 std::shared_ptr<BlockStmt> block = std::make_shared<BlockStmt>();
                 while (!check(TOK_CASE) && !check(TOK_DEFAULT) && !check(TOK_RBRACE) && !isAtEnd()) {
                     block->statements.push_back(declaration());
                 }
                 cases.push_back({nullptr, block});
            } else {
                 // Error or skip
                 advance();
            }
        }
        consume(TOK_RBRACE, "Expect '}' after switch cases.");
        return std::make_shared<SwitchStmt>(condition, cases);
    }
    if (match(TOK_LBRACE)) {
        std::shared_ptr<BlockStmt> block = std::make_shared<BlockStmt>();
        while (!check(TOK_RBRACE) && !isAtEnd()) {
            block->statements.push_back(declaration());
        }
        consume(TOK_RBRACE, "Expect '}'");
        return block;
    }
    std::shared_ptr<Expr> expr = expression();
    match(TOK_SEMICOLON); // optional
    return std::make_shared<ExprStmt>(expr);
}

// Expression Precedence
// expression -> assignment
// assignment -> equality
// ... -> primary

std::shared_ptr<Expr> Parser::expression() {
    return assignment();
}

std::shared_ptr<Expr> Parser::assignment() {
    std::shared_ptr<Expr> expr = logicalOr();
    
    // Ternary operator: condition ? trueExpr : falseExpr
    if (match(TOK_QUESTION)) {
        std::shared_ptr<Expr> trueExpr = expression();
        consume(TOK_COLON, "Expect ':' after true expression in ternary.");
        std::shared_ptr<Expr> falseExpr = assignment();
        return std::make_shared<TernaryExpr>(expr, trueExpr, falseExpr);
    }
    
    if (match(TOK_EQ)) {
        std::shared_ptr<Expr> value = assignment();
        if (auto var = std::dynamic_pointer_cast<VarExpr>(expr)) {
            return std::make_shared<BinaryExpr>(var, "=", value);
        }
        if (auto mem = std::dynamic_pointer_cast<MemberExpr>(expr)) {
            return std::make_shared<BinaryExpr>(mem, "=", value);
        }
    Debugger::parseError("Invalid assignment target.", "", peek().line);
    return expr;
    }
    if (match(TOK_PLUS_EQUAL)) {
        std::shared_ptr<Expr> value = assignment();
        if (auto var = std::dynamic_pointer_cast<VarExpr>(expr)) {
             return std::make_shared<BinaryExpr>(var, "+=", value);
        }
    }
    
    return expr;
}

std::shared_ptr<Expr> Parser::logicalOr() {
    std::shared_ptr<Expr> expr = logicalAnd();
    while (match(TOK_OR)) {
        std::shared_ptr<Expr> right = logicalAnd();
        expr = std::make_shared<BinaryExpr>(expr, "||", right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::logicalAnd() {
    std::shared_ptr<Expr> expr = equality();
    while (match(TOK_AND)) {
        std::shared_ptr<Expr> right = equality();
        expr = std::make_shared<BinaryExpr>(expr, "&&", right);
    }
    return expr;
}

std::shared_ptr<Expr> Parser::equality() {
    std::shared_ptr<Expr> expr = comparison();
    while (match(TOK_EQEQ) || match(TOK_NE)) {
        Token opToken = previous();
        std::string op = opToken.type == TOK_EQEQ ? "==" : "!=";
        std::shared_ptr<Expr> right = comparison();
        auto bin = std::make_shared<BinaryExpr>(expr, op, right);
        bin->line = opToken.line;
        expr = bin;
    }
    return expr;
}

std::shared_ptr<Expr> Parser::comparison() {
    std::shared_ptr<Expr> expr = term();
    while (match(TOK_LT) || match(TOK_GT) || match(TOK_LTE) || match(TOK_GTE)) {
        Token opToken = previous();
        std::string op;
        if (opToken.type == TOK_LT) op = "<";
        else if (opToken.type == TOK_GT) op = ">";
        else if (opToken.type == TOK_LTE) op = "<=";
        else if (opToken.type == TOK_GTE) op = ">=";
        std::shared_ptr<Expr> right = term();
        auto bin = std::make_shared<BinaryExpr>(expr, op, right);
        bin->line = opToken.line;
        expr = bin;
    }
    return expr;
}

std::shared_ptr<Expr> Parser::term() {
    std::shared_ptr<Expr> expr = factor();
    while (match(TOK_PLUS) || match(TOK_MINUS)) {
        std::string op = previous().type == TOK_PLUS ? "+" : "-";
        std::shared_ptr<Expr> right = factor(); 
        auto bin = std::make_shared<BinaryExpr>(expr, op, right);
        bin->line = previous().line;
        expr = bin;
    }
    return expr;
}

std::shared_ptr<Expr> Parser::factor() {
    std::shared_ptr<Expr> expr = unary();
    while (match(TOK_STAR) || match(TOK_SLASH)) {
        std::string op = previous().type == TOK_STAR ? "*" : "/";
        std::shared_ptr<Expr> right = unary();
        auto bin = std::make_shared<BinaryExpr>(expr, op, right);
        bin->line = previous().line;
        expr = bin;
    }
    return expr;
}

std::shared_ptr<Expr> Parser::unary() {
    if (match(TOK_BANG) || match(TOK_MINUS)) {
        Token opToken = previous();
        std::string op = opToken.text;
        std::shared_ptr<Expr> right = unary();
        auto u = std::make_shared<UnaryExpr>(op, right);
        u->line = opToken.line;
        return u;
    }
    return call();
}

std::shared_ptr<Expr> Parser::primary() {
    if (match(TOK_NUMBER)) {
        auto expr = std::make_shared<LiteralExpr>(previous().text, false);
        expr->line = previous().line;
        return expr;
    }
    if (match(TOK_STRING)) {
        auto expr = std::make_shared<LiteralExpr>(previous().text, true);
        expr->line = previous().line;
        return expr;
    }
    
    // Arrow Function: param => ... (Single param, no parens)
    if (check(TOK_IDENTIFIER) && peekNext().type == TOK_ARROW) {
        std::string param = advance().text;
        int line = previous().line;
        consume(TOK_ARROW, "Expect '=>'");
        
        std::shared_ptr<BlockStmt> body = std::make_shared<BlockStmt>();
        if (match(TOK_LBRACE)) {
             while (!check(TOK_RBRACE) && !isAtEnd()) {
                 body->statements.push_back(declaration());
             }
             consume(TOK_RBRACE, "Expect '}'");
        } else {
             // Expression body: param => expr -> implicit return
             std::shared_ptr<Expr> expr = expression();
             body->statements.push_back(std::make_shared<ReturnStmt>(expr));
        }
        auto fn = std::make_shared<FunctionExpr>(std::vector<std::string>{param}, body);
        fn->line = line;
        return fn;
    }

    // Identifier
    if (match(TOK_IDENTIFIER)) {
        auto var = std::make_shared<VarExpr>(previous().text);
        var->line = previous().line;
        return var;
    }
    
    // Object Literal
    if (match(TOK_LBRACE)) {
        Token lbraceToken = previous(); // Capture token for line number
        std::map<std::string, std::shared_ptr<Expr>> props;
        if (!check(TOK_RBRACE)) {
            do {
                // Check for spread operator
                if (match(TOK_DOT_DOT_DOT)) {
                    std::shared_ptr<Expr> spreadExpr = expression();
                    // Store spread with special key prefix
                    static int spreadCounter = 0;
                    auto spread = std::make_shared<SpreadExpr>(spreadExpr);
                    spread->line = previous().line; // Line of '...'
                    props["__spread_" + std::to_string(spreadCounter++)] = spread;
                } else {
                    Token key = consume(TOK_IDENTIFIER, "Expect key.");
                    
                    if (match(TOK_COLON)) {
                         std::shared_ptr<Expr> val = expression();
                         props[key.text] = val;
                    } else {
                         // Shorthand { key } -> { key: key }
                         auto var = std::make_shared<VarExpr>(key.text);
                         var->line = key.line;
                         props[key.text] = var;
                         
                         // If we don't have comma or brace, it might still be error, 
                         // but loop check will handle it.
                    }
                }
            } while (match(TOK_COMMA));
        }
        consume(TOK_RBRACE, "Expect '}' after object literal.");
        auto objExpr = std::make_shared<ObjectExpr>(props);
        objExpr->line = lbraceToken.line; // Line of '{'
        return objExpr;
    }
    
    // Array Literal
    if (match(TOK_LBRACKET)) {
        Token lbracketToken = previous(); // Capture token for line number
        std::vector<std::shared_ptr<Expr>> elements;
        if (!check(TOK_RBRACKET)) {
            do {
                if (match(TOK_DOT_DOT_DOT)) {
                    auto arg = expression();
                    auto spread = std::make_shared<SpreadExpr>(arg);
                    spread->line = previous().line; // Line of '...'
                    elements.push_back(spread);
                } else {
                    elements.push_back(expression());
                }
            } while (match(TOK_COMMA));
        }
        consume(TOK_RBRACKET, "Expect ']' after array literal.");
        auto arrayExpr = std::make_shared<ArrayExpr>(elements);
        arrayExpr->line = lbracketToken.line; // Line of '['
        return arrayExpr;
    }
    

    
    // Grouping or Lambda: ( ) or (a, b) =>
    if (match(TOK_LPAREN)) {
        Token lparenToken = previous(); // Capture token for line number
        // Need to lookahead to see if it is Arrow Function
        // Complex lookahead needed or we speculative parse?
        // Let's assume for now if we see identifiers separated by commas followed by ')' it MIGHT be lambda
        // But (a) is also group expr.
        // We really need to find '=>'
        
        // Simpler approach:
        // If we see ')' immediately -> check '=>' or '{'
        // If 'id' -> peek next?
        
        // Let's stick to simple "Try to parse as lambda if '=>' follows parens" logic, 
        // but since we are one-pass...
        
        // We can check if we hit '=>' AFTER matching parens?
        // We need to parse contents inside parens first.
        // If they are ALL identifiers, we can treat them as potential params.
        
        // LIMITATION: For now, support ONLY empty params `()` or we need robust backtracking.
        // OR: (param) => ...
        // We can peek ahead? `Parser` has `peekNext`.
        
        // Let's implement: `(a) =>` and `(a, b) =>`
        // We already consumed `(`.
        
        // Check if next is `)`.
        if (check(TOK_RPAREN)) {
             // () case
             Token t = peekNext(); // token after )
             if (t.type == TOK_ARROW) {
                 consume(TOK_RPAREN, "Expect ')'");
                 consume(TOK_ARROW, "Expect '=>'");
                  
                  std::shared_ptr<BlockStmt> body = std::make_shared<BlockStmt>();
                  if (match(TOK_LBRACE)) {
                      while (!check(TOK_RBRACE) && !isAtEnd()) {
                          body->statements.push_back(declaration());
                      }
                      consume(TOK_RBRACE, "Expect '}'");
                  } else {
                      // Expression body
                      std::shared_ptr<Expr> expr = expression();
                      body->statements.push_back(std::make_shared<ReturnStmt>(expr));
                  }
                 return std::make_shared<FunctionExpr>(std::vector<std::string>{}, body);
             }
             // Legacy () { ... }
             if (t.type == TOK_LBRACE) {
                 consume(TOK_RPAREN, "Expect ')'");
                 consume(TOK_LBRACE, "Expect '{'");
                 std::shared_ptr<BlockStmt> body = std::make_shared<BlockStmt>();
                 while (!check(TOK_RBRACE) && !isAtEnd()) {
                      body->statements.push_back(declaration());
                 }
                 consume(TOK_RBRACE, "Expect '}'");
                 return std::make_shared<FunctionExpr>(std::vector<std::string>{}, body);
             }
        }
        
        // Try to parse as arrow function parameter list: (a, b) => {...}
        // Lookahead: if we see IDENTIFIER (COMMA IDENTIFIER)* RPAREN ARROW, it's params
        // Try to parse as arrow function parameter list: (a, b) => {...}
        if (check(TOK_IDENTIFIER)) {
            size_t savedPos = current;
            std::vector<std::string> params;
            params.push_back(advance().text);
            
            while (match(TOK_COMMA)) {
                if (check(TOK_IDENTIFIER)) {
                    params.push_back(advance().text);
                } else {
                    current = savedPos;
                    params.clear();
                    break;
                }
            }
            
            if (!params.empty() && check(TOK_RPAREN) && peekNext().type == TOK_ARROW) {
                consume(TOK_RPAREN, "Expect ')'");
                consume(TOK_ARROW, "Expect '=>'");
                
                std::shared_ptr<BlockStmt> body = std::make_shared<BlockStmt>();
                if (match(TOK_LBRACE)) {
                    while (!check(TOK_RBRACE) && !isAtEnd()) {
                        body->statements.push_back(declaration());
                    }
                    consume(TOK_RBRACE, "Expect '}'");
                } else {
                    // Expression body
                    std::shared_ptr<Expr> expr = expression();
                    body->statements.push_back(std::make_shared<ReturnStmt>(expr));
                }
                return std::make_shared<FunctionExpr>(params, body);
            } else {
                // Not arrow function, restore position
                current = savedPos;
            }
        }
        
        
        // If not empty, it's Expression OR Params.
        // Only way to distinguish is `=>`.
        // Let's parse as Expression first.
        std::shared_ptr<Expr> expr = expression();
        consume(TOK_RPAREN, "Expect ')' after expression.");
        
        // If followed by `=>`, we made a mistake?
        // Actually, if expr is `VarExpr`, we can reinterpret it as Param?
        if (match(TOK_ARROW)) {
             // It WAS a lambda. checking if expr is valid param list.
             // Single param `(a)` -> `VarExpr`.
             // Multiple params would be parsed as CallExpr: `(a, b)` -> CallExpr with callee=a, args=[b]
             std::vector<std::string> params;
             if (auto v = std::dynamic_pointer_cast<VarExpr>(expr)) {
                 params.push_back(v->name);
             } else if (auto call = std::dynamic_pointer_cast<CallExpr>(expr)) {
                 // This is actually a parameter list disguised as a call
                 // callee is first param, args are rest
                 if (auto firstParam = std::dynamic_pointer_cast<VarExpr>(call->callee)) {
                     params.push_back(firstParam->name);
                     for (auto& arg : call->args) {
                         if (auto paramVar = std::dynamic_pointer_cast<VarExpr>(arg)) {
                             params.push_back(paramVar->name);
                         }
                     }
                 }
             } else {
                 // Error: (1+1) => ... invalid
                 Debugger::parseError("Invalid parameter list for arrow function.", "", peek().line);
             }
             
             consume(TOK_LBRACE, "Expect '{' for lambda body.");
             std::shared_ptr<BlockStmt> body = std::make_shared<BlockStmt>();
             while (!check(TOK_RBRACE) && !isAtEnd()) {
                 body->statements.push_back(declaration());
             }
             consume(TOK_RBRACE, "Expect '}' after lambda body.");
             return std::make_shared<FunctionExpr>(params, body);
        }
        
        return expr;
    }
    
    // JSX: <Tag ... > ... </Tag> or <Tag ... />
    if (match(TOK_LT)) {
        // Tag Name (Identifier usually, or generic)
        if (!match(TOK_IDENTIFIER)) {
             Debugger::parseError("Expect tag name.", peek().text, peek().line);
        }
        std::string tagName = previous().text;
        std::map<std::string, std::shared_ptr<Expr>> attrs;
        std::vector<std::shared_ptr<Expr>> children;
        
        // Parse attributes: name="value" or name={expr} or boolean name
        // Iterate until '/' or '>'
        while (!check(TOK_GT) && !check(TOK_SLASH) && !isAtEnd()) {
             if (match(TOK_IDENTIFIER)) {
                 std::string key = previous().text;
                 std::shared_ptr<Expr> val = std::make_shared<LiteralExpr>("true", false); // Default boolean true
                 
                 if (match(TOK_EQ)) {
                      if (match(TOK_STRING)) {
                           val = std::make_shared<LiteralExpr>(previous().text, true);
                      } else if (match(TOK_LBRACE)) {
                           val = expression();
                           consume(TOK_RBRACE, "Expect '}' after attribute expression.");
                      } else {
                           Debugger::parseError("Expect string or {expr} for attribute value.", peek().text, peek().line);
                      }
                 }
                 attrs[key] = val;
             } else {
                 advance(); // skip garbage?
             }
        }
        
        // Self closing?
        if (match(TOK_SLASH)) {
            consume(TOK_GT, "Expect '>' after '/' in self-closing tag.");
            return std::make_shared<JsxExpr>(tagName, attrs, children);
        }
        
        consume(TOK_GT, "Expect '>' after attributes.");
        
        // Parse Children
        // Stop at </TagName>
        // We look for '<' '/' 'TagName'
        while (!isAtEnd()) {
            if (check(TOK_LT) && peekNext().type == TOK_SLASH) {
                 // Closing tag candidate
                 // Check if it matches OUR tagName?
                 // We don't peek 2 ahead easily.
                 // Consuming it is safer.
                 break;
            }
            // Text content?
            // If tokens[current] is TEXT... we don't have text tokens.
            // We have generic tokens. 
            // If we hit '<', it's a child JSX. calling primary() will handle '<'.
            // If it is NOT '<', it is text.
            if (check(TOK_LT)) {
                children.push_back(primary());
            } else {
                // Consume as text. Identifier, number, string, symbol...
                // Jsx text handling is tricky without lexer support.
                // Simple: loop consume and append string until '<'?
                // For now, support ONLY JSX child or {Expr} child.
                if (match(TOK_LBRACE)) {
                    if (check(TOK_RBRACE)) {
                        // Empty block {} (e.g. from commented out code)
                        advance(); // consume }
                    } else {
                        children.push_back(expression());
                        consume(TOK_RBRACE, "Expect '}'");
                    }
                } else {
                    // Primitive Text content approximation
                    std::string s = advance().text;
                    children.push_back(std::make_shared<LiteralExpr>(s, true));
                }
            }
        }
        
        consume(TOK_LT, "Expect closing tag.");
        consume(TOK_SLASH, "Expect '/'.");
        if (match(TOK_IDENTIFIER)) {
             if (previous().text != tagName) {
                  Debugger::parseError("Mismatch closing tag: expected " + tagName + ", got " + previous().text, previous().text, previous().line);
             }
        }
        consume(TOK_GT, "Expect '>' after closing tag.");
        return std::make_shared<JsxExpr>(tagName, attrs, children);
    }
    
    Debugger::parseError("Unexpected token.", peek().text, peek().line);
    throw std::runtime_error("Unexpected token in primary expression.");
}

std::shared_ptr<Expr> Parser::call() {
    std::shared_ptr<Expr> expr = primary();
    
    while (true) {
        if (match(TOK_LPAREN)) {
            // Function Call: expr(args)
            std::vector<std::shared_ptr<Expr>> args;
            if (!check(TOK_RPAREN)) {
                do {
                    args.push_back(expression());
                } while (match(TOK_COMMA));
            }
            consume(TOK_RPAREN, "Expect ')' after arguments.");
            auto call = std::make_shared<CallExpr>(expr, args);
            call->line = previous().line; // Closing paren line
            expr = call;
        }
        else if (match(TOK_DOT)) {
            Token name = consume(TOK_IDENTIFIER, "Expect property name after '.'.");
            auto member = std::make_shared<MemberExpr>(expr, std::make_shared<LiteralExpr>(name.text, true), false);
            member->line = name.line;
            expr = member;
        }
        else if (match(TOK_LBRACKET)) {
             auto index = expression();
             consume(TOK_RBRACKET, "Expect ']' after index.");
             Token bracket = previous();
             auto member = std::make_shared<MemberExpr>(expr, index, true);
             member->line = bracket.line;
             expr = member;
        }
        else {
            break;
        }
    }
    
    return expr;
}

// Helpers
bool Parser::match(TokenType t) {
    if (check(t)) {
        advance();
        return true;
    }
    return false;
}
bool Parser::check(TokenType t) {
    if (isAtEnd()) return false;
    return peek().type == t;
}
Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}
bool Parser::isAtEnd() {
    return peek().type == TOK_EOF;
}
Token Parser::peek() {
    return tokens[current];
}
Token Parser::peekNext() {
    if (isAtEnd()) return peek();
    return tokens[current + 1];
}
Token Parser::previous() {
    return tokens[current - 1];
}
Token Parser::consume(TokenType t, std::string err) {
    if (check(t)) return advance();
    Debugger::parseError(err, peek().text, peek().line);
    throw std::runtime_error(err);
}
