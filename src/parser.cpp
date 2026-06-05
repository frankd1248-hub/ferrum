#include "parser.h"

typedef Expr* (*ParseFn) (Parser& parser, bool canAssign);

struct ParseRule {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
};

static Precedence getPrecedence(TokenType type);

static Expr* assign(Parser& p, bool canAssign) {
    VarExpr* target = dynamic_cast<VarExpr*>(p.previousExpr());

    if (!target) {
        p.err.report(p.previous(), "Invalid assignment target.");
        return nullptr;
    }

    auto* expr = new AssignExpr();
    expr->name  = target->name;
    expr->value = p.expression(PREC_ASSIGNMENT);
    return expr;
}

static Expr* binary(Parser& parser, bool canAssign) {
    BinaryExpr* binary = new BinaryExpr();

    binary->op = parser.previous();
    binary->left = parser.previousExpr();
    binary->right = parser.expression(
        (Precedence) (getPrecedence(binary->op.type) + 1)
    );

    return binary;
}

static Expr* boolLit(Parser& parser, bool canAssign) {
    LiteralExpr* literal = new LiteralExpr();
    literal->value = (parser.previous().type == TK_TRUE);
    return literal;
}

static Expr* lparen(Parser& p, bool canAssign) {
    TokenType next = p.peek().type;

    if (next == TK_I32 || next == TK_BOOL || next == TK_VOID) {
        if (p.peekNext().type == TK_RIGHT_PAREN) {
            // it's a cast
            p.advance();
            Type target = p.parseTypeKeyword_prev();  // get Type from previous token
            p.consume(TK_RIGHT_PAREN, "Expect ')' after cast type.");

            auto* cast = new CastExpr();
            cast->token      = p.previous();
            cast->targetType = target;
            cast->expr       = p.expression(PREC_UNARY);
            return cast;
        }
    }

    // otherwise it's a grouped expression
    Expr* inner = p.expression(PREC_NONE);
    p.consume(TK_RIGHT_PAREN, "Expect ')' after expression.");
    return inner;
}

static Expr* number(Parser& parser, bool canAssign) {
    LiteralExpr* literal = new LiteralExpr();
    literal->value = std::stoi(parser.previous().lexeme);
    return literal;
}

static Expr* unary(Parser& parser, bool canAssign) {
    UnaryExpr* unary = new UnaryExpr();

    unary->op = parser.previous();
    unary->right = parser.expression(PREC_UNARY);

    return unary;
}

static Expr* variable(Parser& p, bool canAssign) {
    auto* expr = new VarExpr();
    expr->name = p.previous();
    return expr;
}

static ParseRule rules[] = {
    { nullptr,  nullptr, PREC_NONE       }, // TK_EOF
    { lparen,   nullptr, PREC_CALL       }, // TK_LEFT_PAREN
    { nullptr,  nullptr, PREC_NONE       }, // TK_RIGHT_PAREN
    { nullptr,  nullptr, PREC_NONE       }, // TK_LEFT_BRACE
    { nullptr,  nullptr, PREC_NONE       }, // TK_RIGHT_BRACE
    { nullptr,  nullptr, PREC_NONE       }, // TK_ARROW
    { nullptr,  nullptr, PREC_NONE       }, // TK_SEMICOLON
    { nullptr,  nullptr, PREC_NONE       }, // TK_COLON
    { nullptr,  nullptr, PREC_NONE       }, // TK_COMMA
    { unary,    binary,  PREC_TERM       }, // TK_MINUS
    { nullptr,  binary,  PREC_TERM       }, // TK_PLUS
    { nullptr,  binary,  PREC_FACTOR     }, // TK_STAR
    { nullptr,  binary,  PREC_FACTOR     }, // TK_SLASH
    { nullptr,  assign,  PREC_ASSIGNMENT }, // TK_EQUAL
    { nullptr,  binary,  PREC_COMPARISON }, // TK_EQUAL_EQUAL
    { nullptr,  binary,  PREC_COMPARISON }, // TK_BANG_EQUAL
    { nullptr,  binary,  PREC_COMPARISON }, // TK_LESS
    { nullptr,  binary,  PREC_COMPARISON }, // TK_LESS_EQUAL
    { nullptr,  binary,  PREC_COMPARISON }, // TK_GREATER
    { nullptr,  binary,  PREC_COMPARISON }, // TK_GREATER_EQUAL
    { unary,    nullptr, PREC_NONE       }, // TK_BANG
    { number,   nullptr, PREC_NONE       }, // TK_NUMBER
    { nullptr,  nullptr, PREC_NONE       }, // TK_I32
    { nullptr,  nullptr, PREC_NONE       }, // TK_VOID
    { nullptr,  nullptr, PREC_NONE       }, // TK_BOOL
    { boolLit,  nullptr, PREC_NONE       }, // TK_TRUE
    { boolLit,  nullptr, PREC_NONE       }, // TK_FALSE
    { variable, nullptr, PREC_NONE       }, // TK_IDENTIFIER
    { nullptr,  nullptr, PREC_NONE       }, // TK_LET
    { nullptr,  nullptr, PREC_NONE       }, // TK_CONST
    { nullptr,  nullptr, PREC_NONE       }, // TK_RETURN
    { nullptr,  nullptr, PREC_NONE       }, // TK_FN
    { nullptr,  nullptr, PREC_NONE       }, // TK_IF
    { nullptr,  nullptr, PREC_NONE       }, // TK_ELSE
};

static Precedence getPrecedence(TokenType type) {
    return rules[type].precedence;
}

Expr* Parser::expression(Precedence precedence) {
    advance();

    ParseFn prefixRule = rules[previous().type].prefix;
    if (prefixRule == nullptr) {
        err.report(previous(), "Expect expression.");
        return nullptr;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    Expr* result = prefixRule(*this, canAssign);
    last = result;

    while (precedence < rules[peek().type].precedence) {
        advance();
        ParseFn infixRule = rules[previous().type].infix;
        if (infixRule == nullptr) break;
        result = infixRule(*this, canAssign);
        last = result;
    }

    return result;
}

Type Parser::parseTypeKeyword() {
    if (match(TK_BOOL)) {
        return Type::Boolt;
    } else if (match(TK_I32)) {
        return Type::Int32t;
    } else if (match(TK_VOID)) {
        return Type::Voidt;
    }

    return Type::Nullt;
}

BlockStmt* Parser::block() {
    BlockStmt* block_ = new BlockStmt();

    while (!check(TK_RIGHT_BRACE) && !isAtEnd()) {
        Stmt* s = statement();
        if (s) block_->stmts.push_back(s);
    }

    consume(TK_RIGHT_BRACE, "Expected ending brace for block.");
    return block_;
}

FuncDecl* Parser::fnDeclaration() {
    FuncDecl* fun = new FuncDecl();

    consume(TK_FN, "Expected function declaration.");

    consume(TK_IDENTIFIER, "Expect function name.");
    fun->name = previous();

    consume(TK_LEFT_PAREN, "Expect parameters.");
    // Add parameters parsing here...
    consume(TK_RIGHT_PAREN, "Expect end of parameters.");

    consume(TK_ARROW, "Expect return type.");

    fun->returnType = parseTypeKeyword();

    consume(TK_LEFT_BRACE, "Expected function body.");
    fun->body = block();

    return fun;
}

IfStmt* Parser::ifStatement() {
    IfStmt* stmt = new IfStmt();
    stmt->start = previous();

    consume(TK_LEFT_PAREN, "Expect '(' after 'if'.");
    stmt->condition = expression(PREC_NONE);
    consume(TK_RIGHT_PAREN, "Expect ')' after condition.");

    stmt->thenBranch = statement();

    if (match(TK_ELSE)) {
        stmt->elseBranch = statement();
    }

    return stmt;
}

LetStmt* Parser::letStatement() {
    LetStmt* stmt = new LetStmt();

    do {
        VarDeclarator decl;

        decl.isConst = match(TK_CONST);

        consume(TK_IDENTIFIER, "Expect variable name.");
        decl.name = previous();

        consume(TK_COLON, "Expect ':' after variable name.");
        decl.type = parseTypeKeyword();

        consume(TK_EQUAL, "Expect '=' after type.");
        decl.init = expression(PREC_ASSIGNMENT);

        stmt->declarators.push_back(decl);
    } while (match(TK_COMMA));

    consume(TK_SEMICOLON, "Expect ';' after let statement.");
    return stmt;
}

ReturnStmt* Parser::returnStatement() {
    ReturnStmt* stmt = new ReturnStmt();

    stmt->expr = expression(PREC_NONE);
    consume(TK_SEMICOLON, "Expect ';' after return value.");

    return stmt;
}

Stmt* Parser::statement() {
    if (isAtEnd()) return nullptr;
    if (check(TK_RIGHT_BRACE)) return nullptr;

    if (match(TK_IF))           return ifStatement();
    if (match(TK_LET))          return letStatement();
    if (match(TK_RETURN))       return returnStatement();
    if (match(TK_LEFT_BRACE))   return block();

    Expr* expr = expression(PREC_NONE);
    if (!expr) return nullptr;
    consume(TK_SEMICOLON, "Expect ';' after expression statement.");
    auto* stmt = new ExprStmt();
    stmt->expression = expr;
    return stmt;
}

ASTProgram Parser::parse() {

    program = {};

    FuncDecl* main = fnDeclaration();
    if (main->name.lexeme != "main") {
        err.report(main->name, "Expect 'main' function.");
    }

    program.mainFunction = main;
    return program;
}

Type Parser::parseTypeKeyword_prev() {
    switch (previous().type) {
        case TK_I32:  return Type::Int32t;
        case TK_BOOL: return Type::Boolt;
        case TK_VOID: return Type::Voidt;
        default:      return Type::Nullt;
    }
}