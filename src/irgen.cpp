#include "irgen.h"

IRProgram IRGen::emit(ASTProgram* program) {
    this->AST = program;
    this->AST->mainFunction->accept(*this);
    
    return this->program;
}

void IRGen::emit(IRInstruction inst) {
    currentFunc.body.push_back(inst);
}

IRValue IRGen::newTemp() {
    return IRValue { IRValue::Kind::Temp, tempCount++, -1 };
}

std::string IRGen::newLabel(const std::string& prefix) {
    return prefix + "_" + std::to_string(labelCount++);
}

void IRGen::visit(BlockStmt& block) {
    for (auto* stmt : block.stmts) {
        stmt->accept(*this);
    }
}

void IRGen::visit(ExprStmt& stmt) {
    stmt.expression->accept(*this);
}

void IRGen::visit(FuncDecl& fun) {
    currentFunc = {};
    currentFunc.name       = fun.name.lexeme;
    currentFunc.returnType = fun.returnType;

    fun.body->accept(*this);
    this->program.functions.push_back(currentFunc);
}

void IRGen::visit(IfStmt& stmt) {
    std::string thenLabel = newLabel("then");
    std::string elseLabel = newLabel("else");
    std::string endLabel  = newLabel("end");

    stmt.condition->accept(*this);
    IRValue cond = lastValue;

    // jump to then if true, else fall through to else/end
    emit(IRInstruction { IROp::JumpIf, {}, cond, {}, thenLabel });
    emit(IRInstruction { IROp::Jump,   {}, {}, {},   stmt.elseBranch ? elseLabel : endLabel });

    // then branch
    emit(IRInstruction { IROp::Label, {}, {}, {}, thenLabel });
    stmt.thenBranch->accept(*this);
    emit(IRInstruction { IROp::Jump, {}, {}, {}, endLabel });

    // else branch
    if (stmt.elseBranch) {
        emit(IRInstruction { IROp::Label, {}, {}, {}, elseLabel });
        stmt.elseBranch->accept(*this);
    }

    emit(IRInstruction { IROp::Label, {}, {}, {}, endLabel });
}

void IRGen::visit(LetStmt& stmt) {
    for (auto& decl : stmt.declarators) {
        decl.init->accept(*this); // lastValue = init result

        IRValue dest = newTemp();
        varTemps[decl.name.lexeme] = dest.id;

        emit(IRInstruction { IROp::Mov, dest, lastValue, {} });
        lastValue = dest;
    }
}

void IRGen::visit(ReturnStmt& ret) {
    ret.expr->accept(*this);

    emit(IRInstruction {
        IROp::Ret,
        IRValue {},
        lastValue,
        IRValue {}
    });
}

void IRGen::visit(AssignExpr& expr) {
    expr.value->accept(*this);

    int tempId = varTemps.at(expr.name.lexeme);
    IRValue dest = IRValue { IRValue::Kind::Temp, tempId, -1 };
    emit(IRInstruction { IROp::Mov, dest, lastValue, {} });
    lastValue = dest;
}

void IRGen::visit(BinaryExpr& bin) {
    bin.left->accept(*this);
    IRValue left = lastValue;
    bin.right->accept(*this);
    IRValue right = lastValue;
    IRValue dest = newTemp();

    IROp op;

    switch (bin.op.type) {
        case TK_MINUS: op = IROp::Sub; break;
        case TK_PLUS:  op = IROp::Add; break;
        case TK_STAR:  op = IROp::Mul; break;
        case TK_SLASH: op = IROp::Div; break;
        case TK_EQUAL_EQUAL:   op = IROp::Eq; break;
        case TK_BANG_EQUAL:    op = IROp::Neq; break;
        case TK_LESS:          op = IROp::Less; break;
        case TK_LESS_EQUAL:    op = IROp::Leq; break;
        case TK_GREATER:       op = IROp::Gret; break;
        case TK_GREATER_EQUAL: op = IROp::Geq; break;
    }

    emit(IRInstruction { op, dest, left, right, "" });
    lastValue = dest;
}

void IRGen::visit(CastExpr& cast) {
    cast.expr->accept(*this);

    if (cast.targetType == Type::Boolt &&
        cast.expr->resolvedType == Type::Int32t) {

        IRValue dest = newTemp();
        emit(IRInstruction { IROp::ToBool, dest, lastValue, {} });
        lastValue = dest;
    }

    // Otherwise conversion is not needed
}

void IRGen::visit(LiteralExpr& lit) {
    IRValue dest = newTemp();
    emit(IRInstruction {
        IROp::Const,
        dest,
        IRValue { IRValue::Kind::Constant, -1, 
            std::holds_alternative<int32_t>(lit.value) ? 
                std::get<int32_t>(lit.value) : (int) std::get<bool>(lit.value) },
        IRValue {}, ""
    });
    
    lastValue = dest;
}

void IRGen::visit(UnaryExpr& unary) {
    unary.right->accept(*this);
    IRValue right = lastValue;
    IRValue dest = newTemp();

    IROp op;

    switch (unary.op.type) {
        case TK_MINUS: op = IROp::Sub; break;
        case TK_BANG:  op = IROp::Not; break;
    }

    emit(IRInstruction {
        op,
        dest,
        IRValue { IRValue::Kind::Constant, -1, 0 },
        right, ""
    });
    lastValue = dest;
}

void IRGen::visit(VarExpr& expr) {
    int tempId = varTemps.at(expr.name.lexeme);
    lastValue = IRValue { IRValue::Kind::Temp, tempId, -1 };
}