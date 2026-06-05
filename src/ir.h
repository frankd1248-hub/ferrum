#ifndef ferrum_ir_h
#define ferrum_ir_h

#include "common.h"
#include "ast.h"

struct IRValue {
    enum class Kind { Temp, Constant } kind;
    int id;       // temp number (t0, t1, ...) if Kind::Temp
    int constant; // literal value if Kind::Constant
};

enum class IROp {
    Const,   // dest = constant
    Add, Sub, Mul, Div,  // dest = left op right
    Ret,     // return src
    Label,   // label definition (dest holds label id)
    Jump,    // unconditional jump to label
    JumpIf,  // conditional jump: if src != 0, jump to label
    Call,    // dest = call fn(args...)
    Eq, 
    Neq, 
    Less, 
    Leq, 
    Gret, 
    Geq,    // comparison; result is 0 or 1
    Not, 
    Store,  // store src1 into the slot named by label (the variable name)
    Load,   // dest = value of variable named by label
    Mov,    // dest = src1 (copy)
    ToBool, // dest = (src1 != 0) ? 1 : 0
};

struct IRInstruction {
    IROp     op;
    IRValue  dest;
    IRValue  src1;
    IRValue  src2;
    std::string label; // for Label / Jump / Call ops
};

struct IRFunction {
    std::string name;
    Type        returnType;
    std::vector<std::string>    params; // param names in order
    std::vector<IRInstruction>  body;
};

struct IRProgram {
    std::vector<IRFunction> functions;
};

inline static void printIndents(int amount) {
    for (int i = 0; i < amount; i++) {
        printf("    ");
    }
}

inline std::string getOpString(IROp op) {
    switch (op) {
        case IROp::Const:  return "const";
        case IROp::Add:    return "add";
        case IROp::Sub:    return "sub";
        case IROp::Mul:    return "mul";
        case IROp::Div:    return "div";
        case IROp::Ret:    return "ret";
        case IROp::Label:  return "label";
        case IROp::Jump:   return "jmp";
        case IROp::JumpIf: return "jnz";
        case IROp::Call:   return "call";
        case IROp::Eq:     return "eq";
        case IROp::Neq:    return "neq";
        case IROp::Less:   return "less";
        case IROp::Leq:    return "leq";
        case IROp::Gret:   return "gret";
        case IROp::Geq:    return "geq";
        case IROp::Not:    return "not";
        case IROp::Store:  return "sto";
        case IROp::Load:   return "lod";
        case IROp::Mov:    return "mov";
        case IROp::ToBool: return "bool";
        default: return "nop";
    }
}

inline static std::string stringIRValue(IRValue val) {
    if (val.kind == IRValue::Kind::Temp)
        return "t" + std::to_string(val.id);
    if (val.kind == IRValue::Kind::Constant)
        return std::to_string(val.constant);
    return "?"; // fallback
}

inline void printIRProgram(const IRProgram& program) {
    int indents = 0;

    for (const IRFunction& func : program.functions) {
        printIndents(indents);
        printf("function %s() => %s {\n", func.name.c_str(), TypetoString(func.returnType).c_str());
        indents++;

        for (const IRInstruction& inst : func.body) {
            printIndents(indents);
            printf("%s ", getOpString(inst.op).c_str());

            switch(inst.op) {
                case IROp::Label: printf("%s:", inst.label.c_str()); break;
                case IROp::Add:
                case IROp::Sub:
                case IROp::Mul:
                case IROp::Div:
                case IROp::Eq:
                case IROp::Neq:
                case IROp::Less:
                case IROp::Leq:
                case IROp::Gret:
                case IROp::Geq:
                case IROp::Not:
                    printf("%s, %s %s",
                        stringIRValue(inst.dest).c_str(),
                        stringIRValue(inst.src1).c_str(),
                        stringIRValue(inst.src2).c_str());
                    break;
                case IROp::Jump:
                    printf("=> %s", inst.label.c_str()); break;
                case IROp::JumpIf:
                    printf("%s != 0 ? => %s", 
                        stringIRValue(inst.src1).c_str(), inst.label.c_str());
                    break;
                case IROp::Call: break;
                case IROp::Ret: printf("%s", stringIRValue(inst.src1).c_str()); break;
                case IROp::Const:
                    printf("%s, %s",
                        stringIRValue(inst.dest).c_str(),
                        stringIRValue(inst.src1).c_str());
                    break;
                case IROp::Store:
                    printf("%s <- %s", inst.label.c_str(), stringIRValue(inst.src1).c_str());
                    break;
                case IROp::Load:
                    printf("%s -> %s", inst.label.c_str(), stringIRValue(inst.dest).c_str());
                    break;
                case IROp::Mov:
                    printf("%s, %s", 
                        stringIRValue(inst.dest).c_str(), 
                        stringIRValue(inst.src1).c_str());
                    break;
                case IROp::ToBool:
                    printf("%s <- %s", 
                        stringIRValue(inst.dest).c_str(),
                        stringIRValue(inst.src1).c_str());
                    break;
            }

            printf("\n");
        }

        indents--;
        printIndents(indents);
        printf("}\n");
    }
}

#endif