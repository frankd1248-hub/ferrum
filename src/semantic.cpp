#include "semantic.h"

void SemanticAnalyzer::analyze(ASTProgram& program) {

    for (StructDecl* s : program.structs) {
        StructDef def;
        def.name = s->name.lexeme;
        for (auto& f : s->fields)
            def.fields.push_back(f);
        registry[def.name] = def;
    }

    symbols.pushScope();

    for (FuncDecl* fn : program.functions) {
        Symbol sym;
        sym.name       = fn->name.lexeme;
        sym.type       = fn->returnType;
        sym.isFunction = true;
        sym.declToken  = fn->name;
        for (auto& p : fn->params)
            sym.paramTypes.push_back(p.type);
        symbols.define(sym);
    }

    for (NativeStmt* native : program.natives) {
        Symbol sym;
        sym.name       = native->name.lexeme;
        sym.type       = native->returnType;
        sym.isFunction = true;
        sym.declToken  = native->name;
        for (auto& p : native->params)
            sym.paramTypes.push_back(p.type);
        symbols.define(sym);
    }

    for (FuncDecl* fn : program.functions)
        fn->accept(*this);

    symbols.popScope();
}

void SemanticAnalyzer::visit(BlockStmt& block) {
    symbols.pushScope();

    for (Stmt* stmt : block.stmts) {
        stmt->accept(*this);
    }

    symbols.popScope();
}

void SemanticAnalyzer::visit(BreakStmt& stmt) {
    if (loopDepth == 0)
        err.report(stmt.token, "'break' used outside of a loop.");
}

void SemanticAnalyzer::visit(ContinueStmt& stmt) {
    if (loopDepth == 0)
        err.report(stmt.token, "'continue' used outside of a loop.");
}

void SemanticAnalyzer::visit(ExprStmt& expr) {
    expr.expression->accept(*this);
}

void SemanticAnalyzer::visit(ForStmt& stmt) {
    symbols.pushScope();

    if (stmt.init)
        stmt.init->accept(*this);   // defines i in the new scope

    if (stmt.condition) {
        stmt.condition->accept(*this);
        if (stmt.condition->resolvedType != Type::Boolt)
            err.report(stmt.start, "For condition must be bool.");
    }

    if (stmt.increment)
        stmt.increment->accept(*this);

    loopDepth++;
    stmt.body->accept(*this);
    loopDepth--;

    symbols.popScope();
}

void SemanticAnalyzer::visit(FuncDecl& fn) {
    currentReturnType = fn.returnType;
    symbols.pushScope();

    for (auto& param : fn.params) {
        Symbol sym;
        sym.name      = param.name.lexeme;
        sym.type      = param.type;
        sym.declToken = param.name;
        sym.structName = param.structName;
        symbols.define(sym);
    }

    fn.body->accept(*this);
    symbols.popScope();
}

void SemanticAnalyzer::visit(IfStmt& stmt) {
    stmt.condition->accept(*this);

    if (stmt.condition->resolvedType != Type::Boolt)
        err.report(stmt.start, "If condition must be bool.");

    stmt.thenBranch->accept(*this);

    if (stmt.elseBranch)
        stmt.elseBranch->accept(*this);
}

void SemanticAnalyzer::visit(NativeStmt& stmt) {
    
}

void SemanticAnalyzer::visit(LetStmt& stmt) {
    for (auto& decl : stmt.declarators) {
        if (decl.type == Type::Structt) {
            if (!registry.count(decl.structName))
                err.report(decl.name, "Unknown struct type '" + decl.structName + "'.");
        }

        decl.init->accept(*this);  // resolve init type first

        if (decl.inferred) {
            // infer type from initializer
            decl.type      = decl.init->resolvedType;
            decl.arrayType = getArrayType(decl.init); // see below
        } else {
            if (decl.type == Type::Arrayt) {
                auto* arrLit = dynamic_cast<ArrayLiteral*>(decl.init);
                if (!arrLit) {
                    err.report(decl.name, "Array must be initialized with an array literal.");
                } else if ((int)arrLit->elements.size() != decl.arrayType.size) {
                    err.report(decl.name, "Array literal size does not match declared size.");
                } else if (arrLit->arrayType.elementType != decl.arrayType.elementType) {
                    err.report(decl.name, "Array element type mismatch.");
                }
            } else if (decl.type == Type::Int64t &&
                       decl.init->resolvedType == Type::Int32t) {
                if (auto* lit = dynamic_cast<LiteralExpr*>(decl.init)) {
                    lit->value        = (int64_t)std::get<int32_t>(lit->value);
                    lit->resolvedType = Type::Int64t;
                }
            } else if (decl.init->resolvedType != decl.type) {
                err.report(decl.name, "Type mismatch in declaration.");
            }
        }

        Symbol sym;
        sym.name      = decl.name.lexeme;
        sym.type      = decl.type;
        sym.arrayType = decl.arrayType;
        sym.structName = decl.structName;
        sym.isConst   = decl.isConst;
        sym.declToken = decl.name;
        if (!symbols.define(sym))
            err.report(decl.name, "'" + sym.name + "' already declared in this scope.");
    }
}

ArrayType SemanticAnalyzer::getArrayType(Expr* expr) {
    if (auto* arr = dynamic_cast<ArrayLiteral*>(expr))
        return arr->arrayType;
    return {};
}

void SemanticAnalyzer::visit(ReturnStmt& ret) {
    ret.expr->accept(*this);
    Type actualReturnType = ret.expr->resolvedType;
    if (actualReturnType != currentReturnType) {
        err.report(functionLinestart, 0, "Declared return type does not match actual return type.");
    }
}

void SemanticAnalyzer::visit(WhileStmt& stmt) {
    stmt.condition->accept(*this);
    if (stmt.condition->resolvedType != Type::Boolt)
        err.report(stmt.start, "While condition must be bool.");

    loopDepth++;
    stmt.body->accept(*this);
    loopDepth--;
}

void SemanticAnalyzer::visit(ArrayLiteral& expr) {
    if (expr.elements.empty()) {
        err.report(expr.bracket, "Array literal cannot be empty.");
        expr.resolvedType = Type::Arrayt;
        return;
    }
    expr.elements[0]->accept(*this);
    Type elemType = expr.elements[0]->resolvedType;

    for (size_t i = 1; i < expr.elements.size(); i++) {
        expr.elements[i]->accept(*this);
        if (expr.elements[i]->resolvedType != elemType)
            err.report(expr.bracket, "Array elements must all have the same type.");
    }
    expr.resolvedType = Type::Arrayt;
    expr.arrayType    = { elemType, (int)expr.elements.size() };
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
        case TK_PLUS: 
        case TK_MINUS: 
        case TK_STAR: 
        case TK_SLASH:
            if (L == Type::Float32t || R == Type::Float32t) {
                if (L != Type::Float32t || R != Type::Float32t)
                    err.report(bin.op, "Mixed i32/f32 arithmetic requires an explicit cast.");
                bin.resolvedType = Type::Float32t;
            } if ((L == Type::Int32t || L == Type::Int64t) &&
                (R == Type::Int32t || R == Type::Int64t)) {
                bin.resolvedType = (L == Type::Int64t || R == Type::Int64t)
                    ? Type::Int64t : Type::Int32t;
            } else {
                err.report(bin.op, "Arithmetic operands must be i32 or f32.");
            }
            break;

        case TK_EQUAL_EQUAL: 
        case TK_BANG_EQUAL:
            if (L != R)
                err.report(bin.op, "Operands must have the same type.");
            bin.resolvedType = Type::Boolt;
            break;

        case TK_LESS: 
        case TK_LESS_EQUAL:
        case TK_GREATER: 
        case TK_GREATER_EQUAL:
            if (L != Type::Int32t || R != Type::Int32t)
                err.report(bin.op, "Relational operands must be i32.");
            bin.resolvedType = Type::Boolt;
            break;
        
    } 
}

void SemanticAnalyzer::visit(CallExpr& expr) {
    Symbol* sym = symbols.lookup(expr.callee.lexeme);
    if (!sym) {
        err.report(expr.callee, "Undefined function '" + expr.callee.lexeme + "'.");
        expr.resolvedType = Type::Nullt;
        return;
    } if (!sym->isFunction) {
        err.report(expr.callee, "'" + expr.callee.lexeme + "' is not a function.");
        expr.resolvedType = Type::Nullt;
        return;
    }
    
    if (expr.args.size() != sym->paramTypes.size()) {
        err.report(expr.callee, "Wrong number of arguments.");
        expr.resolvedType = Type::Nullt;
        return;
    }
    for (size_t i = 0; i < expr.args.size(); i++) {
        expr.args[i]->accept(*this);
        if (expr.args[i]->resolvedType != sym->paramTypes[i])
            err.report(expr.callee, "Argument " + std::to_string(i+1) + " type mismatch.");
    }
    expr.resolvedType = sym->type;
}

static bool isCastCompatible(Type from, Type to) {
    if (from == to) return true;

    // From A to B
    static const std::pair<Type,Type> allowed[] = {
        { Type::Int32t,   Type::Int64t },
        { Type::Int64t,   Type::Int32t },
        { Type::Int32t,   Type::Boolt  },
        { Type::Boolt,    Type::Int32t },
        { Type::Int32t,   Type::Float32t },
        { Type::Float32t, Type::Int32t   },
        { Type::Float32t, Type::Boolt    },
        { Type::Boolt,    Type::Float32t },
        { Type::Chart,    Type::Int32t },
        { Type::Int32t,   Type::Chart  },
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

std::string SemanticAnalyzer::getStructName(Expr* expr) {
    if (auto* var = dynamic_cast<VarExpr*>(expr)) {
        Symbol* sym = symbols.lookup(var->name.lexeme);
        if (sym) return sym->structName;
    }
    return "";
}

void SemanticAnalyzer::visit(FieldExpr& expr) {
    expr.object->accept(*this);

    if (expr.object->resolvedType == Type::Structt) {
        // look up struct name from the VarExpr
        std::string sname = getStructName(expr.object);
        if (!registry.count(sname)) {
            err.report(expr.field, "Unknown struct.");
            expr.resolvedType = Type::Nullt;
            return;
        }
        const StructDef& def = registry[sname];
        for (const FieldDef& f : def.fields) {
            if (f.name == expr.field.lexeme) {
                expr.resolvedType = f.type;
                expr.structName   = sname;
                return;
            }
        }
        err.report(expr.field, "Unknown field '" + expr.field.lexeme + "'.");

    } else if (expr.field.lexeme == "len") {
        if (expr.object->resolvedType != Type::Arrayt &&
            expr.object->resolvedType != Type::Stringt)
            err.report(expr.field, "'len' only valid on arrays and Strings.");
        expr.resolvedType = Type::Int32t;
    } else {
        err.report(expr.field, "Field access only valid on structs.");
    }
}

void SemanticAnalyzer::visit(FieldAssignExpr& expr) {
    expr.object->accept(*this);
    expr.value->accept(*this);

    if (expr.object->resolvedType != Type::Structt) {
        err.report(expr.field, "Field assignment only valid on structs.");
        return;
    }
    std::string sname = getStructName(expr.object);
    const StructDef& def = registry.at(sname);
    for (const FieldDef& f : def.fields) {
        if (f.name == expr.field.lexeme) {
            if (expr.value->resolvedType != f.type)
                err.report(expr.field, "Field type mismatch in assignment.");
            expr.resolvedType = f.type;
            return;
        }
    }
    err.report(expr.field, "Unknown field '" + expr.field.lexeme + "'.");
}

void SemanticAnalyzer::visit(IndexExpr& expr) {
    expr.object->accept(*this);
    expr.index->accept(*this);

    if (expr.index->resolvedType != Type::Int32t)
        err.report(expr.bracket, "Index must be i32.");

    if (expr.object->resolvedType == Type::Stringt) {
        expr.elemType     = Type::Chart;
        expr.resolvedType = Type::Chart;
    } else if (expr.object->resolvedType == Type::Arrayt) {
        if (auto* var = dynamic_cast<VarExpr*>(expr.object)) {
            Symbol* sym = symbols.lookup(var->name.lexeme);
            if (sym) {
                expr.elemType     = sym->arrayType.elementType;
                expr.resolvedType = sym->arrayType.elementType;
            }
        }
    } else {
        err.report(expr.bracket, "Index operator only valid on String or array.");
    }
}

void SemanticAnalyzer::visit(IndexAssignExpr& expr) {
    expr.object->accept(*this);
    expr.index->accept(*this);
    expr.value->accept(*this);

    if (expr.object->resolvedType != Type::Arrayt)
        err.report(expr.bracket, "Index assignment only valid on arrays.");
    if (expr.index->resolvedType != Type::Int32t)
        err.report(expr.bracket, "Array index must be i32.");

    expr.resolvedType = expr.value->resolvedType;
}

void SemanticAnalyzer::visit(LiteralExpr& lit) {
    if (std::holds_alternative<int32_t>(lit.value))
        lit.resolvedType = Type::Int32t;
    else if (std::holds_alternative<int64_t>(lit.value))
        lit.resolvedType = Type::Int64t;
    else if (std::holds_alternative<bool>(lit.value))
        lit.resolvedType = Type::Boolt;
    else if (std::holds_alternative<char>(lit.value))
        lit.resolvedType = Type::Chart;
    else if (std::holds_alternative<std::string>(lit.value))
        lit.resolvedType = Type::Stringt;
    else
        lit.resolvedType = Type::Float32t;
}

void SemanticAnalyzer::visit(StructLiteral& expr) {
    if (!registry.count(expr.structName)) {
        err.report(expr.brace, "Unknown struct '" + expr.structName + "'.");
        expr.resolvedType = Type::Nullt;
        return;
    }
    const StructDef& def = registry[expr.structName];

    if (expr.fields.size() != def.fields.size()) {
        err.report(expr.brace, "Wrong number of fields for struct '" +
                   expr.structName + "'.");
    }

    for (size_t i = 0; i < expr.fields.size(); i++) {
        expr.fields[i]->accept(*this);
        if (i < def.fields.size() &&
            expr.fields[i]->resolvedType != def.fields[i].type)
            err.report(expr.brace, "Field '" + def.fields[i].name +
                       "' type mismatch.");
    }

    expr.resolvedType = Type::Structt;
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