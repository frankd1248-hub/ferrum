#include "semantic.h"

void SemanticAnalyzer::analyze(ASTProgram& program) {
    symbols.pushScope();                    // global scope
    program.mainFunction->accept(*this);
    symbols.popScope();
}

void SemanticAnalyzer::visit(BlockStmt& block) {
    symbols.pushScope();

    for (Stmt* stmt : block.stmts) {
        stmt->accept(*this);
    }

    symbols.popScope();
}

void SemanticAnalyzer::visit(FuncDecl& fn) {
    symbols.define({
        fn.name.lexeme, 
        fn.returnType, 
        true, 
        false,
        {},  // Change when adding parameters
        fn.name}
    );

    symbols.pushScope();
    currentReturnType = fn.returnType;
    // Define parameters here
    symbols.popScope();

    functionLinestart = fn.name.line;

    fn.body->accept(*this);
}

void SemanticAnalyzer::visit(IfStmt& stmt) {
    stmt.condition->accept(*this);

    if (stmt.condition->resolvedType != Type::Boolt)
        err.report(stmt.start, "If condition must be bool.");

    stmt.thenBranch->accept(*this);

    if (stmt.elseBranch)
        stmt.elseBranch->accept(*this);
}

void SemanticAnalyzer::visit(LetStmt& stmt) {
    for (auto& decl : stmt.declarators) {
        decl.init->accept(*this);

        if (decl.init->resolvedType != decl.type)
            err.report(decl.name, "Initializer type does not match declared type.");

        Symbol sym;
        sym.name    = decl.name.lexeme;
        sym.type    = decl.type;
        sym.isConst = decl.isConst;
        sym.declToken = decl.name;

        if (!symbols.define(sym))
            err.report(decl.name, "Variable '" + sym.name + "' already declared in this scope.");
    }
}

void SemanticAnalyzer::visit(ReturnStmt& ret) {
    ret.expr->accept(*this);
    Type actualReturnType = ret.expr->resolvedType;
    if (actualReturnType != currentReturnType) {
        err.report(functionLinestart, 0, "Declared return type does not match actual return type.");
    }
}

void SemanticAnalyzer::visit(ExprStmt& expr) {
    expr.expression->accept(*this);
}

void SemanticAnalyzer::visit(AssignExpr& expr) {
    expr.value->accept(*this);

    Symbol* sym = symbols.lookup(expr.name.lexeme);
    if (!sym) {
        err.report(expr.name, "Undefined variable '" + expr.name.lexeme + "'.");
        return;
    }
    if (sym->isConst)
        err.report(expr.name, "Cannot assign to const variable '" + expr.name.lexeme + "'.");
    if (expr.value->resolvedType != sym->type)
        err.report(expr.name, "Assignment type mismatch.");

    expr.resolvedType = sym->type;
}

void SemanticAnalyzer::visit(BinaryExpr& bin) {
    bin.left->accept(*this);
    bin.right->accept(*this);

    Type L = bin.left->resolvedType;
    Type R = bin.right->resolvedType;

    switch (bin.op.type) {
        case TK_PLUS: case TK_MINUS: case TK_STAR: case TK_SLASH:
            if (L != Type::Int32t || R != Type::Int32t)
                err.report(bin.op, "Arithmetic operands must be i32.");
            bin.resolvedType = Type::Int32t;
            break;

        case TK_EQUAL_EQUAL: case TK_BANG_EQUAL:
            if (L != R)
                err.report(bin.op, "Operands must have the same type.");
            bin.resolvedType = Type::Boolt;
            break;

        case TK_LESS: case TK_LESS_EQUAL:
        case TK_GREATER: case TK_GREATER_EQUAL:
            if (L != Type::Int32t || R != Type::Int32t)
                err.report(bin.op, "Relational operands must be i32.");
            bin.resolvedType = Type::Boolt;
            break;
    }
}

static bool isCastCompatible(Type from, Type to) {
    if (from == to) return true;

    // From A to B
    static const std::pair<Type,Type> allowed[] = {
        { Type::Int32t, Type::Boolt  },
        { Type::Boolt,  Type::Int32t },
    };

    for (auto& [f, t] : allowed)
        if (f == from && t == to) return true;

    return false;
}

void SemanticAnalyzer::visit(CastExpr& cast) {
    cast.expr->accept(*this);
    Type from = cast.expr->resolvedType;
    Type to   = cast.targetType;

    if (!isCastCompatible(from, to)) {
        err.report(cast.token,
            "Cannot cast from '" + TypetoString(from) +
            "' to '" + TypetoString(to) + "'.");
        cast.resolvedType = Type::Nullt;
        return;
    }

    cast.resolvedType = to;
}

void SemanticAnalyzer::visit(LiteralExpr& lit) {
    if (std::holds_alternative<int32_t>(lit.value)) {
        lit.resolvedType = Type::Int32t;
    } else if (std::holds_alternative<bool>(lit.value)) {
        lit.resolvedType = Type::Boolt;
    }
}

void SemanticAnalyzer::visit(UnaryExpr& unary) {
    unary.right->accept(*this);

    Type operand = unary.right->resolvedType;

    switch (unary.op.type) {
        case TK_BANG:
            if (operand != Type::Boolt)
                err.report(unary.op, "Operand of '!' must be bool.");
            unary.resolvedType = Type::Boolt;
            break;
        case TK_MINUS:
            if (operand != Type::Int32t)
                err.report(unary.op, "Operand of unary '-' must be i32.");
            unary.resolvedType = Type::Int32t;
            break;
    }
}

void SemanticAnalyzer::visit(VarExpr& expr) {
    Symbol* sym = symbols.lookup(expr.name.lexeme);
    if (!sym) {
        err.report(expr.name, "Undefined variable '" + expr.name.lexeme + "'.");
        expr.resolvedType = Type::Nullt;
        return;
    }
    expr.resolvedType = sym->type;
}