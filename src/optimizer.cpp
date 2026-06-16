#include "optimizer.h"

#include <unordered_set>

void Optimizer::optimize() {
    for (IRFunction& fn : program.functions)
        optimizeFunction(fn);
}

void Optimizer::optimizeFunction(IRFunction& fn) {
    constantFolding(fn.body);
    copyPropagation(fn.body);
    deadCodeElimination(fn.body);
    deadBranchElimination(fn.body);
    emptyLabelElimination(fn.body);
    redundantJumpElimination(fn.body);
    emptyLabelElimination(fn.body);
    unreachableCodeElimination(fn.body);
    copyPropagation(fn.body);
    deadCodeElimination(fn.body);
    emptyLabelElimination(fn.body);
    redundantJumpElimination(fn.body);
    emptyLabelElimination(fn.body);
    unreachableCodeElimination(fn.body);
    deadCodeElimination(fn.body);
}

static bool isSideEffect(IROp op) {
    switch (op) {
        case IROp::Ret:
        case IROp::Jump:
        case IROp::JumpIf:
        case IROp::Label:
        case IROp::Call:
        case IROp::Store:
            return true;
        default:
            return false;
    }
}

void Optimizer::constantFolding(std::vector<IRInstruction>& body) {
    // maps temp id -> its constant value, for temps we've proven are constant
    std::unordered_map<int, IRValue> constants;

    for (IRInstruction& instr : body) {
        // track any Const/FConst dest so we can fold through them
        if ((instr.op == IROp::Const || instr.op == IROp::FConst) &&
             instr.dest.kind == IRValue::Kind::Temp) {
            constants[instr.dest.id] = instr.src1;
        }

        // resolve a value to its known constant if possible
        auto resolve = [&](IRValue val) -> IRValue {
            if (val.kind == IRValue::Kind::Temp && constants.count(val.id))
                return constants[val.id];
            return val;
        };

        IRValue lhs = resolve(instr.src1);
        IRValue rhs = resolve(instr.src2);

        bool lhsConst = lhs.kind == IRValue::Kind::IntConst ||
                        lhs.kind == IRValue::Kind::FloatConst;
        bool rhsConst = rhs.kind == IRValue::Kind::IntConst ||
                        rhs.kind == IRValue::Kind::FloatConst;

        if (!lhsConst || !rhsConst) continue;

        // both operands are constant — fold
        IRValue result;
        bool folded = true;

        if (lhs.kind == IRValue::Kind::FloatConst ||
            rhs.kind == IRValue::Kind::FloatConst) {
            // float folding
            float l = lhs.kind == IRValue::Kind::FloatConst ? lhs.fval : (float)lhs.ival;
            float r = rhs.kind == IRValue::Kind::FloatConst ? rhs.fval : (float)rhs.ival;
            float val;
            switch (instr.op) {
                case IROp::FAdd: val = l + r; break;
                case IROp::FSub: val = l - r; break;
                case IROp::FMul: val = l * r; break;
                case IROp::FDiv:
                    if (r == 0.0f) { folded = false; break; } // don't fold div by zero
                    val = l / r; break;
                default: folded = false; break;
            }
            if (folded)
                result = IRValue { .kind = IRValue::Kind::FloatConst, .id = -1, .fval = val };
        } else {
            // integer folding
            int l = lhs.ival;
            int r = rhs.ival;
            int val;
            switch (instr.op) {
                case IROp::Add: val = l + r; break;
                case IROp::Sub: val = l - r; break;
                case IROp::Mul: val = l * r; break;
                case IROp::Div:
                    if (r == 0) { folded = false; break; } // don't fold div by zero
                    val = l / r; break;
                // comparison ops — result is 0 or 1
                case IROp::Eq:   val = (l == r) ? 1 : 0; break;
                case IROp::Neq:  val = (l != r) ? 1 : 0; break;
                case IROp::Less: val = (l <  r) ? 1 : 0; break;
                case IROp::Leq:  val = (l <= r) ? 1 : 0; break;
                case IROp::Gret: val = (l >  r) ? 1 : 0; break;
                case IROp::Geq:  val = (l >= r) ? 1 : 0; break;
                default: folded = false; break;
            }
            if (folded)
                result = IRValue { .kind = IRValue::Kind::IntConst, .id = -1, .ival = val };
        }

        if (!folded) continue;

        // rewrite the instruction as a Const
        bool isFloat = result.kind == IRValue::Kind::FloatConst;
        instr.op   = isFloat ? IROp::FConst : IROp::Const;
        instr.src1 = result;
        instr.src2 = IRValue {};

        // record the new constant so downstream instructions can fold through it
        if (instr.dest.kind == IRValue::Kind::Temp)
            constants[instr.dest.id] = result;
    }
}

void Optimizer::deadCodeElimination(std::vector<IRInstruction>& body) {
    std::unordered_set<int> used;

    for (const IRInstruction& instr : body) {
        if (instr.src1.kind == IRValue::Kind::Temp) used.insert(instr.src1.id);
        if (instr.src2.kind == IRValue::Kind::Temp) used.insert(instr.src2.id);
        for (const IRValue& arg : instr.args)
            if (arg.kind == IRValue::Kind::Temp) used.insert(arg.id);
    }

    std::vector<IRInstruction> result;
    for (const IRInstruction& instr : body) {
        bool hasDest = instr.dest.kind == IRValue::Kind::Temp;
        bool destUsed = hasDest && used.count(instr.dest.id);

        if (!hasDest || destUsed || isSideEffect(instr.op))
            result.push_back(instr);
    }

    body = std::move(result);
}

void Optimizer::copyPropagation(std::vector<IRInstruction>& body) {
    std::unordered_map<int, IRValue> aliases;
    std::unordered_map<int, IRValue> constants;

    std::unordered_map<int, int> defCount;
    for (const IRInstruction& instr : body) {
        if (instr.dest.kind == IRValue::Kind::Temp)
            defCount[instr.dest.id]++;
    }

    for (const IRInstruction& instr : body) {
        if (instr.op == IROp::Mov &&
            instr.dest.kind == IRValue::Kind::Temp &&
            defCount[instr.dest.id] == 1) {
            aliases[instr.dest.id] = instr.src1;
        }
        // ← track Const and FConst dests as known constants
        if ((instr.op == IROp::Const || instr.op == IROp::FConst) &&
            instr.dest.kind == IRValue::Kind::Temp &&
            defCount[instr.dest.id] == 1) {
            constants[instr.dest.id] = instr.src1;
        }
    }

    auto resolve = [&](IRValue val) -> IRValue {
        int limit = 32;
        while (val.kind == IRValue::Kind::Temp && limit-- > 0) {
            if (aliases.count(val.id))      { val = aliases[val.id]; continue; }
            if (constants.count(val.id))    { val = constants[val.id]; break; }
            break;
        }
        return val;
    };

    for (IRInstruction& instr : body) {
        instr.src1 = resolve(instr.src1);
        instr.src2 = resolve(instr.src2);
    }
}

void Optimizer::deadBranchElimination(std::vector<IRInstruction>& body) {
    std::unordered_map<std::string, int> labelIndex;
    for (int i = 0; i < (int)body.size(); i++)
        if (body[i].op == IROp::Label)
            labelIndex[body[i].label] = i;

    std::vector<IRInstruction> result;
    for (int i = 0; i < (int)body.size(); i++) {
        const IRInstruction& instr = body[i];

        if (instr.op == IROp::JumpIf &&
            instr.src1.kind == IRValue::Kind::IntConst) {
            if (instr.src1.ival != 0) {
                result.push_back({ IROp::Jump, {}, {}, {}, instr.label });
            }
            continue;
        }

        result.push_back(instr);
    }

    body = std::move(result);
}

void Optimizer::unreachableCodeElimination(std::vector<IRInstruction>& body) {
    std::vector<IRInstruction> result;
    bool reachable = true;

    for (const IRInstruction& instr : body) {
        if (instr.op == IROp::Label) {
            reachable = true;
        }

        if (reachable)
            result.push_back(instr);

        if (instr.op == IROp::Jump ||
            instr.op == IROp::Ret) {
            reachable = false;
        }
    }

    body = std::move(result);
}

void Optimizer::emptyLabelElimination(std::vector<IRInstruction>& body) {
    std::unordered_set<std::string> referenced;
    for (const IRInstruction& instr : body)
        if (instr.op == IROp::Jump || instr.op == IROp::JumpIf)
            referenced.insert(instr.label);

    std::vector<IRInstruction> result;
    for (const IRInstruction& instr : body) {
        if (instr.op == IROp::Label && !referenced.count(instr.label))
            continue;
        result.push_back(instr);
    }

    body = std::move(result);
}

void Optimizer::strengthReduction(std::vector<IRInstruction>& body) {
    for (IRInstruction& instr : body) {
        // x * 1 → x 
        // x * 0 → 0 
        // x + 0 → x
        // x - 0 → x
        if (instr.op == IROp::Mul) {
            IRValue lhs = instr.src1, rhs = instr.src2;
            if (rhs.kind == IRValue::Kind::IntConst) {
                if (rhs.ival == 1) {
                    instr.op   = IROp::Mov;
                    instr.src1 = lhs;
                    instr.src2 = {};
                } else if (rhs.ival == 0) {
                    instr.op   = IROp::Const;
                    instr.src1 = IRValue { .kind=IRValue::Kind::IntConst, .id=-1, .ival=0 };
                    instr.src2 = {};
                }
            }
            if (lhs.kind == IRValue::Kind::IntConst && lhs.ival == 0) {
                instr.op   = IROp::Const;
                instr.src1 = IRValue { .kind=IRValue::Kind::IntConst, .id=-1, .ival=0 };
                instr.src2 = {};
            }
        }
        if (instr.op == IROp::Add || instr.op == IROp::Sub) {
            IRValue rhs = instr.src2;
            if (rhs.kind == IRValue::Kind::IntConst && rhs.ival == 0) {
                instr.op   = IROp::Mov;
                instr.src2 = {};
            }
        }
    }
}

void Optimizer::redundantJumpElimination(std::vector<IRInstruction>& body) {
    std::vector<IRInstruction> result;

    for (int i = 0; i < (int)body.size(); i++) {
        const IRInstruction& instr = body[i];

        if (instr.op == IROp::Jump && i + 1 < (int)body.size()) {
            const IRInstruction& next = body[i + 1];
            if (next.op == IROp::Label && next.label == instr.label)
                continue;
        }

        result.push_back(instr);
    }

    body = std::move(result);
}