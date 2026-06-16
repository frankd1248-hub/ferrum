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

static Expr* call(Parser& p, bool canAssign) {
    VarExpr* callee = dynamic_cast<VarExpr*>(p.previousExpr());
    if (!callee) {
        p.err.report(p.previous(), "Expected function name before '('.");
        return nullptr;
    }

    CallExpr* expr = new CallExpr();
    expr->callee = callee->name;

    if (!p.check(TK_RIGHT_PAREN)) {
        do {
            expr->args.push_back(p.expression(PREC_ASSIGNMENT));
        } while (p.match(TK_COMMA));
    }
    p.consume(TK_RIGHT_PAREN, "Expect ')' after arguments.");
    return expr;
}

static Expr* charLit(Parser& p, bool canAssign) {
    LiteralExpr* lit = new LiteralExpr();
    lit->value = p.previous().lexeme[0];
    return lit;
}

static Expr* floatLit(Parser& parser, bool canAssign) {
    LiteralExpr* literal = new LiteralExpr();
    literal->value = std::stof(parser.previous().lexeme);
    return literal;
}

static Expr* index(Parser& p, bool canAssign) {
    IndexExpr* expr = new IndexExpr();
    expr->object  = p.previousExpr();
    expr->bracket = p.previous();
    expr->index   = p.expression(PREC_NONE);

    p.consume(TK_RIGHT_BRACKET, "Expect ']' after index.");
    return expr;
}

static Expr* lparen(Parser& parser, bool canAssign) {
    TokenType next = parser.peek().type;

    if (next == TK_I32 || next == TK_F32 || next == TK_BOOL || next == TK_VOID) {
        if (parser.peekNext().type == TK_RIGHT_PAREN) {
            // it's a cast
            parser.advance();
            Type target = parser.parseTypeKeyword_prev();
            parser.consume(TK_RIGHT_PAREN, "Expect ')' after cast type.");

            auto* cast = new CastExpr();
            cast->token      = parser.previous();
            cast->targetType = target;
            cast->expr       = parser.expression(PREC_UNARY);
            return cast;
        }
    }

    // otherwise it's a grouped expression
    Expr* inner = parser.expression(PREC_NONE);
    parser.consume(TK_RIGHT_PAREN, "Expect ')' after expression.");
    return inner;
}

static Expr* number(Parser& parser, bool canAssign) {
    LiteralExpr* literal = new LiteralExpr();
    literal->value = std::stoi(parser.previous().lexeme);
    return literal;
}

static Expr* strLit(Parser& p, bool canAssign) {
    LiteralExpr* lit = new LiteralExpr();
    lit->value = p.previous().lexeme;
    return lit;
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
    { lparen,   call,    PREC_CALL       }, // TK_LEFT_PAREN
    { nullptr,  nullptr, PREC_NONE       }, // TK_RIGHT_PAREN
    { nullptr,  index,   PREC_CALL       }, // TK_LEFT_BRACKET
    { nullptr,  nullptr, PREC_NONE       }, // TK_RIGHT_BRACKET
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
    { floatLit, nullptr, PREC_NONE       }, // TK_FLOAT
    { charLit,  nullptr, PREC_NONE       }, // TK_CHAR_LIT
    { strLit,   nullptr, PREC_NONE       }, // TK_STRING_LIT
    { nullptr,  nullptr, PREC_NONE       }, // TK_I32
    { nullptr,  nullptr, PREC_NONE       }, // TK_F32
    { nullptr,  nullptr, PREC_NONE       }, // TK_VOID
    { nullptr,  nullptr, PREC_NONE       }, // TK_BOOL
    { nullptr,  nullptr, PREC_NONE       }, // TK_CHAR
    { nullptr,  nullptr, PREC_NONE       }, // TK_STRING
    { boolLit,  nullptr, PREC_NONE       }, // TK_TRUE
    { boolLit,  nullptr, PREC_NONE       }, // TK_FALSE
    { variable, nullptr, PREC_NONE       }, // TK_IDENTIFIER
    { nullptr,  nullptr, PREC_NONE       }, // TK_LET
    { nullptr,  nullptr, PREC_NONE       }, // TK_CONST
    { nullptr,  nullptr, PREC_NONE       }, // TK_BREAK
    { nullptr,  nullptr, PREC_NONE       }, // TK_CONTINUE
    { nullptr,  nullptr, PREC_NONE       }, // TK_RETURN
    { nullptr,  nullptr, PREC_NONE       }, // TK_FN
    { nullptr,  nullptr, PREC_NONE       }, // TK_IF
    { nullptr,  nullptr, PREC_NONE       }, // TK_ELSE
    { nullptr,  nullptr, PREC_NONE       }, // TK_FOR
    { nullptr,  nullptr, PREC_NONE       }, // TK_WHILE 
    { nullptr,  nullptr, PREC_NONE       }, // TK_NATIVE
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
    if      (match(TK_BOOL))   return Type::Boolt;
    else if (match(TK_CHAR))   return Type::Chart;
    else if (match(TK_I32))    return Type::Int32t;
    else if (match(TK_F32))    return Type::Float32t;
    else if (match(TK_STRING)) return Type::Stringt;
    else if (match(TK_VOID))   return Type::Voidt;

    return Type::Nullt;
}

Type Parser::parseTypeKeyword_prev() {
    switch (previous().type) {
        case TK_BOOL:   return Type::Boolt;
        case TK_CHAR:   return Type::Chart;
        case TK_I32:    return Type::Int32t;
        case TK_F32:    return Type::Float32t;
        case TK_STRING: return Type::Stringt;
        case TK_VOID:   return Type::Voidt;
        default:        return Type::Nullt;
    }
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

BreakStmt* Parser::breakStatement() {
    BreakStmt* stmt = new BreakStmt();
    stmt->token = previous();
    consume(TK_SEMICOLON, "Expect ';' after 'break'.");
    return stmt;
}

ContinueStmt* Parser::continueStatement() {
    ContinueStmt* stmt = new ContinueStmt();
    stmt->token = previous();
    consume(TK_SEMICOLON, "Expect ';' after 'continue'.");
    return stmt;
}

ForStmt* Parser::forStatement() {
    ForStmt* stmt = new ForStmt();
    stmt->start = previous();

    consume(TK_LEFT_PAREN, "Expect '(' after 'for'.");

    if (match(TK_LET)) {
        stmt->init = letStatement(false);
    } else {
        stmt->init = nullptr;
    }
    consume(TK_SEMICOLON, "Expect ';' after for initializer.");

    if (!check(TK_SEMICOLON))
        stmt->condition = expression(PREC_NONE);
    consume(TK_SEMICOLON, "Expect ';' after for condition.");

    if (!check(TK_RIGHT_PAREN))
        stmt->increment = expression(PREC_NONE);

    consume(TK_RIGHT_PAREN, "Expect ')' after for clauses.");
    stmt->body = statement();
    return stmt;
}

FuncDecl* Parser::fnDeclaration() {
    FuncDecl* fun = new FuncDecl();
    consume(TK_FN, "Expected 'fn'.");
    consume(TK_IDENTIFIER, "Expect function name.");
    fun->name = previous();

    consume(TK_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(TK_RIGHT_PAREN)) {
        do {
            consume(TK_IDENTIFIER, "Expect parameter name.");
            Token paramName = previous();
            consume(TK_COLON, "Expect ':' after parameter name.");
            Type paramType = parseTypeKeyword();
            fun->params.push_back({ paramName, paramType });
        } while (match(TK_COMMA));
    }
    consume(TK_RIGHT_PAREN, "Expect ')' after parameters.");

    consume(TK_ARROW, "Expect '=>' before return type.");
    fun->returnType = parseTypeKeyword();

    consume(TK_LEFT_BRACE, "Expect '{' before function body.");
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

NativeStmt* Parser::nativeStatement() {
    NativeStmt* stmt = new NativeStmt();

    consume(TK_IDENTIFIER, "Expect native function name.");
    stmt->name = previous();

    consume(TK_LEFT_PAREN, "Expect '(' after native function name.");
    if (!check(TK_RIGHT_PAREN)) {
        do {
            consume(TK_IDENTIFIER, "Expect parameter name.");
            Token paramName = previous();
            consume(TK_COLON, "Expect ':' after parameter name.");
            Type paramType = parseTypeKeyword();
            stmt->params.push_back({ paramName, paramType });
        } while (match(TK_COMMA));
    }
    consume(TK_RIGHT_PAREN, "Expect ')' after parameters.");

    consume(TK_ARROW, "Expect '=>' before return type.");
    stmt->returnType = parseTypeKeyword();

    return stmt;
}

LetStmt* Parser::letStatement(bool consumeSemicolon) {
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

    if (consumeSemicolon)
        consume(TK_SEMICOLON, "Expect ';' after let statement.");
    return stmt;
}

ReturnStmt* Parser::returnStatement() {
    ReturnStmt* stmt = new ReturnStmt();

    stmt->expr = expression(PREC_NONE);
    consume(TK_SEMICOLON, "Expect ';' after return value.");

    return stmt;
}

WhileStmt* Parser::whileStatement() {
    WhileStmt* stmt = new WhileStmt();
    stmt->start = previous();

    consume(TK_LEFT_PAREN, "Expect '(' after 'while'.");
    stmt->condition = expression(PREC_NONE);
    consume(TK_RIGHT_PAREN, "Expect ')' after condition.");

    stmt->body = statement();
    return stmt;
}

Stmt* Parser::statement() {
    if (isAtEnd()) return nullptr;
    if (check(TK_RIGHT_BRACE)) return nullptr;

    if (match(TK_BREAK))        return breakStatement();
    if (match(TK_CONTINUE))     return continueStatement();
    if (match(TK_FOR))          return forStatement();
    if (match(TK_IF))           return ifStatement();
    if (match(TK_LET))          return letStatement();
    if (match(TK_RETURN))       return returnStatement();
    if (match(TK_WHILE))        return whileStatement();
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
    while (!isAtEnd()) {
        if (match(TK_NATIVE)) {
            consume(TK_FN, "Expect 'fn' after 'native'.");
            NativeStmt* native = nativeStatement();
            consume(TK_SEMICOLON, "Expect ';' after native declaration.");
            program.natives.push_back(native);
        } else {
            FuncDecl* fn = fnDeclaration();
            program.functions.push_back(fn);
            if (fn->name.lexeme == "main") {
                if (!program.mainFunction)
                    program.mainFunction = fn;
                else
                    err.report(fn->name, "Cannot have more than one 'main'.");
            }
        }
    }
    if (!program.mainFunction)
        err.report(peek(), "No 'main' function defined.");
    return program;
}