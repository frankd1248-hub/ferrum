#ifndef cuprum_ir_h
#define cuprum_ir_h

#include "common.h"
#include "ast.h"

struct IRValue {
    enum class Kind { None, Temp, IntConst, FloatConst } kind = Kind::None;
    int   id   = -1;
    Type  type = Type::Nullt;
    std::string label;
    union { int64_t ival = 0; float fval; };
};

enum class IROp {
    Const,        // dest = constant
    FConst,       // dest = float constant (uses src1.fval)
    StringConst,  // dest = pointer to string literal (address in rax)
    CharConst,    // dest = char literal
    Index,        // dest = string[index] — loads one byte
    Add, 
    Sub, 
    Mul, 
    Div,          // dest = left op right
    FAdd, 
    FSub, 
    FMul, 
    FDiv,
    Ret,          // return src
    Label,        // label definition (dest holds label id)
    Jump,         // unconditional jump to label
    JumpIf,       // conditional jump: if src != 0, jump to label
    Call,         // dest = call fn(args...)
    Param,        // dest = param N (index stored in src1.ival)
    Eq, 
    Neq, 
    Less, 
    Leq, 
    Gret, 
    Geq,          // comparison; result is 0 or 1
    Not, 
    Store,        // store src1 into the slot named by label (the variable name)
    Load,         // dest = value of variable named by label
    Mov,          // dest = src1 (copy)
    ToFloat,      // dest = (f32) src1  — i32 → f32 conversion
    ToInt,        // dest = (i32) src1  — f32 → i32 conversion (truncates)
    ToBool,      
    ArrayAlloc,   // dest = base ptr of N-element array
                  // src1 = IntConst element count
    ArrayStore,   // MEM[dest + src1 * 8] = src2
                  // dest = base ptr, src1 = index, src2 = value
    ArrayLoad,    // dest = MEM[src1 + src2 * 8]
                  // src1 = base ptr, src2 = index 
    StructAlloc,
    StructStore,
    StructLoad,
};

struct IRInstruction {
    IROp     op;
    IRValue  dest;
    IRValue  src1;
    IRValue  src2;
    std::string          label;  // function name for Call, label name for jumps
    std::vector<IRValue> args;   // for Call only
};

struct IRParam {
    std::string name;
    Type        type;
};

struct IRFunction {
    std::string          name;
    Type                 returnType;
    std::vector<IRParam> params;
    std::vector<IRInstruction> body;
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
        case IROp::Const:       return "const";
        case IROp::FConst:      return "constf";
        case IROp::CharConst:   return "constc";
        case IROp::StringConst: return "consts";
        case IROp::Index:       return "index";
        case IROp::Add:         return "add";
        case IROp::Sub:         return "sub";
        case IROp::Mul:         return "mul";
        case IROp::Div:         return "div";
        case IROp::FAdd:        return "addf";
        case IROp::FSub:        return "subf";
        case IROp::FMul:        return "mulf";
        case IROp::FDiv:        return "divf";
        case IROp::Ret:         return "ret";
        case IROp::Label:       return "label";
        case IROp::Jump:        return "jmp";
        case IROp::JumpIf:      return "jnz";
        case IROp::Call:        return "call";
        case IROp::Param:       return "param";
        case IROp::Eq:          return "eq";
        case IROp::Neq:         return "neq";
        case IROp::Less:        return "less";
        case IROp::Leq:         return "leq";
        case IROp::Gret:        return "gret";
        case IROp::Geq:         return "geq";
        case IROp::Not:         return "not";
        case IROp::Store:       return "sto";
        case IROp::Load:        return "lod";
        case IROp::Mov:         return "mov";
        case IROp::ToFloat:     return "float";
        case IROp::ToInt:       return "int";
        case IROp::ToBool:      return "bool";
        case IROp::ArrayAlloc:  return "alloca";
        case IROp::ArrayStore:  return "stoa";
        case IROp::ArrayLoad:   return "loda";
        case IROp::StructAlloc: return "allocs";
        case IROp::StructStore: return "stos";
        case IROp::StructLoad:  return "stol";
        default:                return "nop";
    }
}

inline static std::string stringIRValue(IRValue val) {
    if (val.kind == IRValue::Kind::Temp)
        return "t" + std::to_string(val.id);
    if (val.kind == IRValue::Kind::IntConst)
        return std::to_string(val.ival);
    if (val.kind == IRValue::Kind::FloatConst)
        return std::to_string(val.fval);
    return "?"; // fallback
}

inline void printIRProgram(const IRProgram& program) {
    int indents = 0;

    for (const IRFunction& func : program.functions) {
        printIndents(indents);
        printf("\nfunction %s() => %s {\n", func.name.c_str(), TypetoString(func.returnType).c_str());
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
                case IROp::FAdd:
                case IROp::FSub:
                case IROp::FMul:
                case IROp::FDiv:
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
                case IROp::Call:
                    printf("%s <- %s(", stringIRValue(inst.dest).c_str(), inst.label.c_str());
                    for (size_t i = 0; i < inst.args.size(); i++) {
                        printf("%s", stringIRValue(inst.args[i]).c_str());
                        if (i + 1 < inst.args.size()) printf(", ");
                    }
                    printf(")");
                    break;
                case IROp::Param:
                    printf("%s <- params[%d]", stringIRValue(inst.dest).c_str(),
                        inst.src1.ival); break;
                case IROp::Ret: printf("%s", stringIRValue(inst.src1).c_str()); break;
                case IROp::Const:
                case IROp::FConst:
                case IROp::CharConst:
                    printf("%s, %s",
                        stringIRValue(inst.dest).c_str(),
                        stringIRValue(inst.src1).c_str());
                    break;
                case IROp::StringConst:
                    printf("%s, lbl %s", 
                        stringIRValue(inst.dest).c_str(), inst.label.c_str());
                    break;
                case IROp::Index:
                    printf("%s <- lbl %s[%s]",
                        stringIRValue(inst.dest).c_str(),
                        inst.label.c_str(), stringIRValue(inst.src1).c_str());
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
                case IROp::ToFloat:
                case IROp::ToInt:
                    printf("%s <- %s", 
                        stringIRValue(inst.dest).c_str(),
                        stringIRValue(inst.src1).c_str());
                    break;
                case IROp::ArrayAlloc:
                case IROp::StructAlloc:
                    printf("%s <- ptr[%d]",
                        stringIRValue(inst.dest).c_str(), inst.src1.ival);
                    break;
                case IROp::ArrayStore:
                case IROp::StructStore:
                    printf("%s[%s] <- %s",
                        stringIRValue(inst.dest).c_str(),
                        stringIRValue(inst.src1).c_str(),
                        stringIRValue(inst.src2).c_str());
                    break;
                case IROp::ArrayLoad:
                case IROp::StructLoad:
                    printf("%s <- %s[%s]",
                        stringIRValue(inst.dest).c_str(),
                        stringIRValue(inst.src1).c_str(),
                        stringIRValue(inst.src2).c_str());
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