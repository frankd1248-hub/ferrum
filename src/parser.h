#ifndef cuprum_parser_h
#define cuprum_parser_h

#include "common.h"
#include "ast.h"
#include "error_reporter.h"
#include "token.h"

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,    // =
    PREC_TERNARY,       // ?
    PREC_OR,            // ||
    PREC_AND,           // &&
    PREC_BITWISE_OR,    // |
    PREC_BITWISE_XOR,   // ^
    PREC_BITWISE_AND,   // &
    PREC_EQUALITY,      // == !=
    PREC_COMPARISON,    // < > <= >=
    PREC_BITWISE_SHIFT, // << >>
    PREC_TERM,          // + -
    PREC_FACTOR,        // * /
    PREC_UNARY,         // ! - ~
    PREC_CALL,          // . () []
    PREC_PRIMARY
} Precedence;

class Parser {
public:

    Parser(std::vector<Token> tokens, ErrorReporter& err) : tokens(tokens), err(err) { }

    ASTProgram parse();

    Token peek() const {
        if (current >= (int) tokens.size()) return { TK_EOF, "", 0, 0 };
        return tokens[current];
    }

    Token peekNext() const {
        if (current + 1 >= (int) tokens.size()) return { TK_EOF, "", 0, 0 };
        return tokens[current + 1];
    }

    Token previous() const {
        if (current == 0) return { TK_EOF, "", 0, 0 };
        return tokens[current - 1];
    }

    Expr* previousExpr() const {
        return last;
    }

    bool isAtEnd() const {
        return peek().type == TK_EOF;
    }

    void advance() {
        if (!isAtEnd()) current++;
    }

    bool check(TokenType type) const {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    bool match(TokenType type) {
        if (check(type)) {
            advance();
            return true;
        }
        return false;
    }

    void consume(TokenType type, const std::string& msg) {
        if (!match(type)) {
            err.report(peek(), msg);
        }
    }

    Expr* expression(Precedence precedence);
    Stmt* statement();

    Type parseTypeKeyword();
    Type parseTypeKeyword_prev();

    ErrorReporter& err;

private:
    std::vector<Token> tokens;

    ASTProgram program;

    int current = 0;
    
    Expr* last = nullptr;

    BlockStmt*    block();
    BreakStmt*    breakStatement();
    ContinueStmt* continueStatement();
    ForStmt*      forStatement();
    FuncDecl*     fnDeclaration();
    IfStmt*       ifStatement();
    NativeStmt*   nativeStatement();
    LetStmt*      letStatement(bool consumeSemicolon = true);
    ReturnStmt*   returnStatement();
    WhileStmt*    whileStatement();
};

#endif