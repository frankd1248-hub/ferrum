#include "parser.h"

typedef Expr* (*ParseFn) (Parser& parser, bool canAssign);

struct ParseRule {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
};

static Precedence getPrecedence(TokenType type);

static Expr* arrayLit(Parser& p, bool canAssign) {
    auto* lit = new ArrayLiteral();
    lit->bracket = p.previous();
    if (!p.check(TK_RIGHT_BRACKET)) {
        do {
            lit->elements.push_back(p.expression(PREC_ASSIGNMENT));
        } while (p.match(TK_COMMA));
    }
    p.consume(TK_RIGHT_BRACKET, "Expect ']' after array literal.");
    return lit;
}

static Expr* assign(Parser& p, bool canAssign) {
    VarExpr* target = dynamic_cast<VarExpr*>(p.previousExpr());

    if (!target) {
        p.error(p.previous(), "Invalid assignment target.");
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
        p.error(p.previous(), "Expected function name before '('.");
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

static Expr* dot(Parser& p, bool canAssign) {
    p.consume(TK_IDENTIFIER, "Expect field name after '.'.");
    Token field = p.previous();

    if (canAssign && p.match(TK_EQUAL)) {
        FieldAssignExpr* assign  = new FieldAssignExpr();
        assign->object = p.previousExpr();
        assign->field  = field;
        assign->value  = p.expression(PREC_ASSIGNMENT);
        return assign;
    }

    FieldExpr* expr   = new FieldExpr();
    expr->object = p.previousExpr();
    expr->field  = field;
    return expr;
}

static Expr* floatLit(Parser& parser, bool canAssign) {
    LiteralExpr* literal = new LiteralExpr();
    literal->value = std::stof(parser.previous().lexeme);
    return literal;
}

static Expr* index(Parser& p, bool canAssign) {
    Expr*  object  = p.previousExpr();
    Token  bracket = p.previous();
    Expr*  idx     = p.expression(PREC_NONE);
    p.consume(TK_RIGHT_BRACKET, "Expect ']' after index.");

    if (canAssign && p.match(TK_EQUAL)) {
        IndexAssignExpr* assign    = new IndexAssignExpr();
        assign->object  = object;
        assign->index   = idx;
        assign->bracket = bracket;
        assign->value   = p.expression(PREC_ASSIGNMENT);
        return assign;
    }

    IndexExpr* expr    = new IndexExpr();
    expr->object  = object;
    expr->index   = idx;
    expr->bracket = bracket;
    return expr;
}

static Expr* lnumber(Parser& parser, bool canAssign) {
    LiteralExpr* literal = new LiteralExpr();
    literal->value = (int64_t) std::stoll(parser.previous().lexeme);
    return literal;
}

static Expr* lparen(Parser& parser, bool canAssign) {
    TokenType next = parser.peek().type;

    if (next == TK_I32 || next == TK_I64 || next == TK_F32 || 
        next == TK_BOOL || next == TK_VOID) {
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
    literal->value = (int32_t) std::stoi(parser.previous().lexeme);
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
    { arrayLit, index,   PREC_CALL       }, // TK_LEFT_BRACKET
    { nullptr,  nullptr, PREC_NONE       }, // TK_RIGHT_BRACKET
    { nullptr,  nullptr, PREC_NONE       }, // TK_LEFT_BRACE
    { nullptr,  nullptr, PREC_NONE       }, // TK_RIGHT_BRACE
    { nullptr,  nullptr, PREC_NONE       }, // TK_ARROW
    { nullptr,  nullptr, PREC_NONE       }, // TK_SEMICOLON
    { nullptr,  nullptr, PREC_NONE       }, // TK_COLON
    { nullptr,  nullptr, PREC_NONE       }, // TK_COMMA
    { nullptr,  dot,     PREC_CALL       }, // TK_DOT
    { unary,    binary,  PREC_TERM       }, // TK_MINUS
    { nullptr,  binary,  PREC_TERM       }, // TK_PLUS
    { nullptr,  binary,  PREC_FACTOR     }, // TK_STAR
    { nullptr,  binary,  PREC_FACTOR     }, // TK_SLASH
    { nullptr,  assign,  PREC_ASSIGNMENT }, // TK_EQUAL
    { nullptr,  binary,  PREC_EQUALITY   }, // TK_EQUAL_EQUAL
    { nullptr,  binary,  PREC_EQUALITY   }, // TK_BANG_EQUAL
    { nullptr,  binary,  PREC_COMPARISON }, // TK_LESS
    { nullptr,  binary,  PREC_COMPARISON }, // TK_LESS_EQUAL
    { nullptr,  binary,  PREC_COMPARISON }, // TK_GREATER
    { nullptr,  binary,  PREC_COMPARISON }, // TK_GREATER_EQUAL
    { unary,    nullptr, PREC_NONE       }, // TK_BANG
    { number,   nullptr, PREC_NONE       }, // TK_NUMBER
    { lnumber,  nullptr, PREC_NONE       }, // TK_LNUMBER
    { floatLit, nullptr, PREC_NONE       }, // TK_FLOAT
    { charLit,  nullptr, PREC_NONE       }, // TK_CHAR_LIT
    { strLit,   nullptr, PREC_NONE       }, // TK_STRING_LIT
    { nullptr,  nullptr, PREC_NONE       }, // TK_I32
    { nullptr,  nullptr, PREC_NONE       }, // TK_I64
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
    { nullptr,  nullptr, PREC_NONE       }, // TK_STRUCT
};

static Precedence getPrecedence(TokenType type) {
    return rules[type].precedence;
}

Expr* Parser::expression(Precedence precedence) {
    advance();
    ParseFn prefixRule = rules[previous().type].prefix;
    if (prefixRule == nullptr) {
        error(previous(), "Expect expression.");
        return nullptr;
    }

    bool canAssign = precedence <= PREC_ASSIGNMENT;
    Expr* result = prefixRule(*this, canAssign);
    if (!result) return nullptr;
    last = result;

    while (precedence < rules[peek().type].precedence) {
        advance();
        ParseFn infixRule = rules[previous().type].infix;
        if (infixRule == nullptr) break;
        result = infixRule(*this, canAssign);
        if (!result) return nullptr;
        last = result;
    }

    return result;
}

Type Parser::parseTypeKeyword() {
    if (match(TK_I32))    return Type::Int32t;
    if (match(TK_I64))    return Type::Int64t;
    if (match(TK_F32))    return Type::Float32t;
    if (match(TK_BOOL))   return Type::Boolt;
    if (match(TK_CHAR))   return Type::Chart;
    if (match(TK_STRING)) return Type::Stringt;
    if (match(TK_VOID))   return Type::Voidt;
    if (match(TK_IDENTIFIER)) return Type::Structt; // struct type by name
    return Type::Nullt;
}

Type Parser::parseTypeKeyword_prev() {
    switch (previous().type) {
        case TK_BOOL:   return Type::Boolt;
        case TK_CHAR:   return Type::Chart;
        case TK_I32:    return Type::Int32t;
        case TK_I64:    return Type::Int64t;
        case TK_F32:    return Type::Float32t;
        case TK_STRING: return Type::Stringt;
        case TK_IDENTIFIER: return Type::Structt;
        case TK_VOID:   return Type::Voidt;
        default:        return Type::Nullt;
    }
}

std::pair<Type, ArrayType> Parser::parseTypeAnnotation() {
    if (match(TK_LEFT_BRACKET)) {
        Type elem = parseTypeKeyword();
        consume(TK_SEMICOLON, "Expect ';' in array type.");
        consume(TK_NUMBER, "Expect array size.");
        int size = std::stoi(previous().lexeme);
        consume(TK_RIGHT_BRACKET, "Expect ']' after array type.");
        return { Type::Arrayt, { elem, size } };
    }
    return { parseTypeKeyword(), {} };
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
            auto [type, arrayType] = parseTypeAnnotation();
            fun->params.push_back(Param {
                paramName, type, arrayType,
                type == Type::Structt ? previous().lexeme : ""  // structName
            });
        } while (match(TK_COMMA));
    }
    consume(TK_RIGHT_PAREN, "Expect ')' after parameters.");

    consume(TK_ARROW, "Expect '=>' before return type.");
    auto [type, arrayType] = parseTypeAnnotation();
    fun->returnType      = type;
    fun->returnArrayType = arrayType;

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

StructLiteral* Parser::structLiteralExpr(const std::string& structName) {
    auto* lit = new StructLiteral();
    lit->brace      = peek();
    lit->structName = structName;

    consume(TK_LEFT_BRACE, "Expect '{' for struct literal.");
    if (!check(TK_RIGHT_BRACE)) {
        do {
            lit->fields.push_back(expression(PREC_ASSIGNMENT));
        } while (match(TK_COMMA));
    }
    consume(TK_RIGHT_BRACE, "Expect '}' after struct literal.");
    return lit;
}

LetStmt* Parser::letStatement(bool consumeSemicolon) {
    auto* stmt = new LetStmt();

    do {
        VarDeclarator decl;
        decl.isConst = match(TK_CONST);

        consume(TK_IDENTIFIER, "Expect variable name.");
        decl.name = previous();

        if (match(TK_COLON)) {
            // explicit type annotation
            auto [type, arrayType] = parseTypeAnnotation();
            decl.type      = type;
            decl.arrayType = arrayType;
            decl.inferred  = false;

            if (type == Type::Structt)
                decl.structName = previous().lexeme;
        } else {
            // no annotation — infer from initializer
            decl.type     = Type::Nullt;
            decl.inferred = true;
        }

        consume(TK_EQUAL, "Expect '=' after variable name.");
        if (decl.type == Type::Structt && check(TK_LEFT_BRACE)) {
            decl.init = structLiteralExpr(decl.structName);
        } else {
            decl.init = expression(PREC_ASSIGNMENT);
        }

        stmt->declarators.push_back(decl);
    } while (match(TK_COMMA));

    if (consumeSemicolon)
        consume(TK_SEMICOLON, "Expect ';' after let statement.");
    return stmt;
}

StructDecl* Parser::structDeclaration() {
    StructDecl* decl = new StructDecl();

    consume(TK_IDENTIFIER, "Expect struct name.");
    decl->name = previous();

    consume(TK_LEFT_BRACE, "Expect '{' after struct name.");

    while (!check(TK_RIGHT_BRACE) && !isAtEnd()) {
        FieldDef field;
        consume(TK_IDENTIFIER, "Expect field name.");
        field.name = previous().lexeme;
        consume(TK_COLON, "Expect ':' after field name.");
        auto [type, arrayType] = parseTypeAnnotation();
        field.type      = type;
        field.arrayType = arrayType;
        // if type is a struct, the identifier is the struct name
        if (type == Type::Structt)
            field.structName = previous().lexeme;
        decl->fields.push_back(field);
    }

    consume(TK_RIGHT_BRACE, "Expect '}' after struct fields.");
    consume(TK_SEMICOLON, "Expect ';' after struct declaration.");
    return decl;
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

    if (match(TK_IF))     return ifStatement();
    if (match(TK_FOR))    return forStatement();
    if (match(TK_WHILE))  return whileStatement();
    if (match(TK_LET))    return letStatement();
    if (match(TK_RETURN)) return returnStatement();
    if (match(TK_BREAK))  return breakStatement();
    if (match(TK_CONTINUE)) return continueStatement();
    if (match(TK_LEFT_BRACE)) return block();

    Expr* expr = expression(PREC_NONE);
    if (!expr) {
        synchronize();
        return nullptr;
    }
    consume(TK_SEMICOLON, "Expect ';' after expression statement.");
    ExprStmt* stmt = new ExprStmt();
    stmt->expression = expr;
    return stmt;
}

ASTProgram Parser::parse() {
    program = {};
    while (!isAtEnd()) {
        if (match(TK_STRUCT)) {
            program.structs.push_back(structDeclaration());
        } else if (match(TK_NATIVE)) {
            consume(TK_FN, "Expect 'fn' after 'native'.");
            program.natives.push_back(nativeStatement());
            consume(TK_SEMICOLON, "Expect ';' after native declaration.");
        } else {
            FuncDecl* fn = fnDeclaration();
            if (fn) {
                program.functions.push_back(fn);
                if (fn->name.lexeme == "main")
                    program.mainFunction = fn;
            }
        }
    }
    return program;
}

void Parser::synchronize() {
    panicMode = false;

    while (!isAtEnd()) {
        
        if (previous().type == TK_SEMICOLON) return;

        switch (peek().type) {
            case TK_FN:
            case TK_NATIVE:
            case TK_LET:
            case TK_IF:
            case TK_FOR:
            case TK_WHILE:
            case TK_RETURN:
            case TK_BREAK:
            case TK_CONTINUE:
            case TK_LEFT_BRACE:
            case TK_RIGHT_BRACE: // end of a block — useful for nested errors
                return;
            default:
                advance();
        }
    }
}