#include "irgen.h"

IRProgram IRGen::emit(ASTProgram* program) {
    this->AST = program;

    for (FuncDecl* fn : this->AST->functions)
        fn->accept(*this);

    return this->program;
}

void IRGen::emit(IRInstruction inst) {
    currentFunc.body.push_back(inst);
}

IRValue IRGen::newTemp() {
    return IRValue { IRValue::Kind::Temp, tempCount++, Type::Nullt, -1 };
}

std::string IRGen::newLabel(const std::string& prefix) {
    return prefix + "_" + std::to_string(labelCount++);
}

std::string IRGen::stringLabel(const std::string& value) {
    if (!stringLabels.count(value)) {
        std::string lbl = ".LCs" + std::to_string(stringLabels.size());
        stringLabels[value] = lbl;
    }
    return stringLabels[value];
}

void IRGen::visit(BlockStmt& block) {
    for (auto* stmt : block.stmts) {
        stmt->accept(*this);
    }
}

void IRGen::visit(ExprStmt& stmt) {
    stmt.expression->accept(*this);
}

/**
 * IR Structure;
 * 
 * [init]
 * label cond:
 *     [condition] -> t0
 *     jnz t0, body
 *     jmp end
 * label body:
 *     [body]
 *     [increment]
 *     jmp cond
 * label end:
 */
void IRGen::visit(ForStmt& stmt) {
    std::string condLabel = newLabel("cond");
    std::string bodyLabel = newLabel("body");
    std::string endLabel  = newLabel("end");

    if (stmt.init) stmt.init->accept(*this);

    emit({ IROp::Label, {}, {}, {}, condLabel });

    if (stmt.condition) {
        stmt.condition->accept(*this);
        emit({ IROp::JumpIf, {}, lastValue, {}, bodyLabel });
        emit({ IROp::Jump,   {}, {}, {},         endLabel  });
    }

    emit({ IROp::Label, {}, {}, {}, bodyLabel });
    stmt.body->accept(*this);

    if (stmt.increment) stmt.increment->accept(*this);
    emit({ IROp::Jump, {}, {}, {}, condLabel });

    emit({ IROp::Label, {}, {}, {}, endLabel });
}

void IRGen::visit(FuncDecl& fn) {
    currentFunc = {};
    currentFunc.name       = fn.name.lexeme;
    currentFunc.returnType = fn.returnType;

    for (size_t i = 0; i < fn.params.size(); i++) {
        currentFunc.params.push_back({ fn.params[i].name.lexeme, fn.params[i].type });

        IRValue dest = newTemp();
        varTemps[fn.params[i].name.lexeme] = dest.id;

        // Param N → dest temp
        emit({ IROp::Param, dest,
               IRValue { .kind = IRValue::Kind::IntConst, .id = -1, .ival = (int)i },
               {}, "" });
    }

    fn.body->accept(*this);
    program.functions.push_back(currentFunc);
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

/**
 * IR Structure:
 * 
 * label cond:
 *     [condition] -> t0
 *     jnz t0, body
 *     jmp end
 * label body:
 *     [body]
 *     jmp cond
 * label end:
 */
void IRGen::visit(WhileStmt& stmt) {
    std::string condLabel = newLabel("cond");
    std::string bodyLabel = newLabel("body");
    std::string endLabel  = newLabel("end");

    emit({ IROp::Label, {}, {}, {}, condLabel });

    stmt.condition->accept(*this);
    emit({ IROp::JumpIf, {}, lastValue, {}, bodyLabel });
    emit({ IROp::Jump,   {}, {}, {},         endLabel  });

    emit({ IROp::Label, {}, {}, {}, bodyLabel });
    stmt.body->accept(*this);
    emit({ IROp::Jump,  {}, {}, {}, condLabel });

    emit({ IROp::Label, {}, {}, {}, endLabel });
}

void IRGen::visit(AssignExpr& expr) {
    expr.value->accept(*this);

    int tempId = varTemps.at(expr.name.lexeme);
    IRValue dest = IRValue { IRValue::Kind::Temp, tempId, lastValue.type, -1 };
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

    bool isFloat = bin.resolvedType == Type::Float32t;

    switch (bin.op.type) {
        case TK_PLUS:  op = (isFloat ? IROp::FAdd : IROp::Add); break;
        case TK_MINUS: op = (isFloat ? IROp::FSub : IROp::Sub); break;
        case TK_STAR:  op = (isFloat ? IROp::FMul : IROp::Mul); break;
        case TK_SLASH: op = (isFloat ? IROp::FDiv : IROp::Div); break;
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

void IRGen::visit(CallExpr& expr) {
    std::vector<IRValue> argVals;
    for (size_t i = 0; i < expr.args.size(); i++) {
        expr.args[i]->accept(*this);
        IRValue arg  = lastValue;
        arg.type     = expr.args[i]->resolvedType;
        argVals.push_back(arg);
    }

    IRValue dest = newTemp();
    IRInstruction call;
    call.op    = IROp::Call;
    call.dest  = dest;
    call.label = expr.callee.lexeme;
    call.args  = argVals;
    emit(call);

    lastValue = dest;
}

void IRGen::visit(CastExpr& cast) {
    cast.expr->accept(*this);

    Type from = cast.expr->resolvedType;
    Type to   = cast.targetType;

    if (from == to) return;

    IRValue dest = newTemp();
    IROp op;

    if (to == Type::Int32t && from == Type::Chart) return;

    if      (to == Type::Boolt)    op = IROp::ToBool;
    else if (to == Type::Float32t) op = IROp::ToFloat;
    else if (to == Type::Int32t)   op = IROp::ToInt;

    emit(IRInstruction { op, dest, lastValue, {} });
    lastValue = dest;
}

void IRGen::visit(IndexExpr& expr) {
    expr.object->accept(*this);
    IRValue strPtr = lastValue;

    expr.index->accept(*this);
    IRValue idx = lastValue;

    IRValue dest = newTemp();
    emit({ IROp::Index, dest, strPtr, idx, "" });
    lastValue = dest;
}

void IRGen::visit(LiteralExpr& lit) {
    IRValue dest = newTemp();

    if (std::holds_alternative<_Float32>(lit.value)) {
        float fval = std::get<_Float32>(lit.value);
        emit(IRInstruction {
            IROp::FConst, dest,
            IRValue { .kind=IRValue::Kind::FloatConst, .id=-1, .fval=fval },
            {}
        });
    } else if (std::holds_alternative<int32_t>(lit.value) ||
               std::holds_alternative<bool>(lit.value)) {
        int ival = std::holds_alternative<int32_t>(lit.value)
            ? std::get<int32_t>(lit.value)
            : (int) std::get<bool>(lit.value);
        emit(IRInstruction {
            IROp::Const, dest,
            IRValue { .kind=IRValue::Kind::IntConst, .id=-1, .ival = ival },
            {}
        });
    } else if (std::holds_alternative<char>(lit.value)) {
        emit({ IROp::Const, dest,
            IRValue { .kind=IRValue::Kind::IntConst, .id=-1, .ival=(int)(unsigned char)std::get<char>(lit.value) },
            {}, "" });
        lastValue = dest;
    } else if (std::holds_alternative<std::string>(lit.value)) {
        const std::string& val = std::get<std::string>(lit.value);
        std::string lbl = stringLabel(val);
        emit({ IROp::StringConst, dest, {}, {}, lbl });
        lastValue = dest;
    }

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

    bool isFloat = unary.resolvedType == Type::Float32t;

    emit(IRInstruction {
        op,
        dest,
        isFloat ? IRValue { .kind=IRValue::Kind::FloatConst, .id=-1, .type=Type::Float32t, .fval=0.0f } : 
                  IRValue { .kind=IRValue::Kind::IntConst,   .id=-1, .type=Type::Int32t,   .ival=0},
        right, ""
    });
    lastValue = dest;
}

void IRGen::visit(VarExpr& expr) {
    int tempId = varTemps.at(expr.name.lexeme);
    lastValue = IRValue { IRValue::Kind::Temp, tempId, Type::Nullt, -1 };
}