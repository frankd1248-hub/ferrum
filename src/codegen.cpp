#include "codegen.h"

#include <cstring>

void CodeGen::emit(const std::string& line) {
    out << "\t" << line << "\n";
}

void CodeGen::emitLabel(const std::string& label) {
    out << label << ":\n";
}

std::string CodeGen::stackRef(int tempId) const {
    return "QWORD PTR [rbp-" + std::to_string(frame.offsetOf(tempId)) + "]";
}

std::string CodeGen::fStackRef(int tempId) const {
    return "DWORD PTR [rbp-" + std::to_string(frame.offsetOf(tempId)) + "]";
}

std::string CodeGen::resolve(const IRValue& val) const {
    if (val.kind == IRValue::Kind::Temp) {
        auto it = regMap.find(val.id);
        if (it != regMap.end())
            return it->second;
        return stackRef(val.id);
    }

    if (val.kind == IRValue::Kind::IntConst)
        return std::to_string(val.ival);
    if (val.kind == IRValue::Kind::FloatConst)
        return std::to_string(val.fval);
    return "?";
}

std::string CodeGen::floatLabel(float fval) {
    if (!floatLabels.count(fval)) {
        std::string label = ".LCf" + std::to_string(floatLabels.size());
        floatLabels[fval] = label;
    }

    return floatLabels[fval];
}

void CodeGen::emitFloatPool() {
    if (floatLabels.empty()) return;
    out << ".section .rodata\n";
    for (auto& [fval, label] : floatLabels) {
        out << label << ":\n";
        
        uint32_t bits;
        memcpy(&bits, &fval, 4);
        out << "\t.long " << bits << "\n";
    }
    out << ".text\n";
}

void CodeGen::emitStringPool() {
    if (stringLabels.empty()) return;
    out << ".section .rodata\n";
    for (auto& [value, label] : stringLabels) {
        out << label << "_len:\n";
        out << "\t.quad " << value.size() << "\n";
        out << label << ":\n";
        out << "\t.string \"";
        for (unsigned char c : value) {
            if      (c == '"')  out << "\\\"";
            else if (c == '\\') out << "\\\\";
            else if (c == '\n') out << "\\n";
            else if (c == '\t') out << "\\t";
            else if (c == '\r') out << "\\r";
            else if (c < 32 || c > 126)
                out << "\\x" << std::hex << (int)c << std::dec;
            else out << c;
        }
        out << "\"\n";
    }
    out << ".text\n";
}

void CodeGen::generate(const std::string& outputPath) {
    out.open(outputPath);
    emitPreamble();
    for (const IRFunction& fn : program.functions)
        emitFunction(fn);
    emitFloatPool();
    emitStringPool();
    out.close();
}

void CodeGen::emitPreamble() {
    out << ".intel_syntax noprefix\n\n";

    out << ".global _start\n";
    emitLabel("_start");
    emit("call\tmain");
    emit("mov\trdi, rax");   // exit code = return value of main
    emit("mov\trax, 60");    // syscall number: exit
    emit("syscall");
    out << "\n";
}


void CodeGen::emitFunction(const IRFunction& fn) {
    frame.reset();
    paramTempIds.clear();
    currentFunc = &fn;

    regMap = regAlloc.allocate(fn);

    for (const IRInstruction& instr : fn.body) {
        if (instr.op == IROp::ArrayAlloc) {
            frame.allocate(instr.dest.id);           // pointer slot
            int dataBase = frame.nextFree();         // where data starts
            for (int i = 0; i < instr.src1.ival; i++)
                frame.allocateAnon();                // reserve data slots
            arrayBases[instr.dest.id] = dataBase;
        } else if (instr.op == IROp::StructAlloc) {
            int count = instr.src1.ival;
            int base  = frame.allocateArray(instr.dest.id, count);
            arrayBases[instr.dest.id] = base;
        } else {
            if (instr.dest.kind == IRValue::Kind::Temp && !regMap.contains(instr.dest.id))
                frame.allocate(instr.dest.id);
            if (instr.op == IROp::Param)
                paramTempIds.push_back(instr.dest.id);
        }

        for (const IRValue& arg : instr.args) {
            if (arg.kind == IRValue::Kind::Temp && !regMap.contains(arg.id) && !frame.hasSlot(arg.id))
                frame.allocate(arg.id);
        }
    }

    out << ".global " << fn.name << "\n";
    emitLabel(fn.name);
    emitPrologue(fn, frame.frameSize());

    for (const IRInstruction& instr : fn.body)
        emitInstruction(instr);
}

void CodeGen::emitPrologue(const IRFunction& fn, int frameSize) {
    static const std::vector<std::string> calleeSaved =
        { "rbx", "r12", "r13", "r14", "r15" };

    savedRegs.clear();
    for (auto& [id, reg] : regMap)
        if (std::find(calleeSaved.begin(), calleeSaved.end(), reg)
            != calleeSaved.end())
            if (std::find(savedRegs.begin(), savedRegs.end(), reg)
                == savedRegs.end())
                savedRegs.push_back(reg);

    for (const std::string& reg : savedRegs)
        emit("push\t" + reg);

    emit("push\trbp");
    emit("mov\trbp, rsp");

    if (frameSize > 0)
        emit("sub\trsp, " + std::to_string(frameSize));
}

void CodeGen::emitEpilogue() {
    emit("mov\trsp, rbp");
    emit("pop\trbp");
    for (auto it = savedRegs.rbegin(); it != savedRegs.rend(); ++it)
        emit("pop\t" + *it);
    emit("ret");
}

void CodeGen::emitInstruction(const IRInstruction& instr) {
    switch (instr.op) {
        case IROp::Const:  emitConst(instr);  break;
        case IROp::FConst: emitFConst(instr); break;
        case IROp::CharConst:
            emit("mov\trax, " + std::to_string(instr.src1.ival));
            emit("mov\t" + resolve(instr.dest) + ", rax");
            break;
        case IROp::StringConst: {
            emit("lea\trax, [" + instr.label + " + rip]");
            emit("mov\t" + resolve(instr.dest) + ", rax");
            break;
        }
        case IROp::Index: emitIndex(instr); break;
        case IROp::Ret:    emitRet(instr);    break;
        case IROp::Add:
        case IROp::Sub:
        case IROp::Mul:
        case IROp::Div:    emitBinop(instr);  break;
        case IROp::FAdd:
        case IROp::FSub:
        case IROp::FMul:
        case IROp::FDiv:   emitFBinop(instr); break;
        case IROp::Label:  emitLabel(instr.label); break;
        case IROp::Eq:     emitCmp(instr, "sete");  break;
        case IROp::Neq:    emitCmp(instr, "setne"); break;
        case IROp::Less:   emitCmp(instr, "setl");  break;
        case IROp::Leq:    emitCmp(instr, "setle"); break;
        case IROp::Gret:   emitCmp(instr, "setg");  break;
        case IROp::Geq:    emitCmp(instr, "setge"); break;
        case IROp::Not:    emitNot(instr); break;
        case IROp::Jump:   emit("jmp\t" + instr.label); break;
        case IROp::JumpIf:
            emit("cmp\t" + resolve(instr.src1) + ", 0");
            emit("jne\t" + instr.label);
            break;
        case IROp::Call:  emitCall(instr); break;
        case IROp::Param: emitParam(instr); break;
        case IROp::Mov:
            emit("mov\trax, " + resolve(instr.src1));
            emit("mov\t" + resolve(instr.dest) + ", rax");
            break;
        case IROp::ToBool:
            emit("mov\trax, " + resolve(instr.src1));
            emit("cmp\trax, 0");
            emit("setne\tal");
            emit("movzx\trax, al");
            emit("mov\t" + resolve(instr.dest) + ", rax");
            break;
        case IROp::ToFloat:     emitToFloat(instr); break;
        case IROp::ToInt:       emitToInt(instr); break;
        case IROp::ArrayAlloc:  emitArrayAlloc(instr); break;
        case IROp::ArrayStore:  emitArrayStore(instr); break;
        case IROp::ArrayLoad:   emitArrayLoad(instr);  break;
        case IROp::StructAlloc: emitArrayAlloc(instr); break;
        case IROp::StructStore: emitArrayStore(instr); break;
        case IROp::StructLoad:  emitArrayLoad(instr);  break;
    }
}

void CodeGen::emitArrayAlloc(const IRInstruction& instr) {
    int dataBase = arrayBases[instr.dest.id];
    emit("lea\trax, [rbp-" + std::to_string(dataBase) + "]");
    emit("mov\t" + resolve(instr.dest) + ", rax");
}

void CodeGen::emitArrayStore(const IRInstruction& instr) {
    emit("mov\trax, " + resolve(instr.dest));  // base ptr
    emit("mov\trcx, " + resolve(instr.src1));  // index
    emit("mov\trdx, " + resolve(instr.src2));  // value
    emit("mov\tQWORD PTR [rax + rcx*8], rdx");
}

void CodeGen::emitArrayLoad(const IRInstruction& instr) {
    emit("mov\trax, " + resolve(instr.src1));  // base ptr
    emit("mov\trcx, " + resolve(instr.src2));  // index
    emit("mov\trax, QWORD PTR [rax + rcx*8]");
    emit("mov\t" + resolve(instr.dest) + ", rax");
}

void CodeGen::emitBinop(const IRInstruction& instr) {
    std::string dest = resolve(instr.dest);
    emit("mov\trax, " + resolve(instr.src1));
    emit("mov\tr9,  " + resolve(instr.src2));

    switch (instr.op) {
        case IROp::Add: emit("add\trax, r9");  break;
        case IROp::Sub: emit("sub\trax, r9");  break;
        case IROp::Mul: emit("imul\trax, r9"); break;
        case IROp::Div: emit("cqo"); emit("idiv\tr9"); break;
        default: break;
    }
    emit("mov\t" + dest + ", rax");
}

void CodeGen::emitFBinop(const IRInstruction& instr) {
    emit("mov\trax, " + resolve(instr.src1));
    emit("movq\txmm0, rax");
    emit("mov\trax, " + resolve(instr.src2));
    emit("movq\txmm1, rax");

    switch (instr.op) {
        case IROp::FAdd: emit("addss\txmm0, xmm1"); break;
        case IROp::FSub: emit("subss\txmm0, xmm1"); break;
        case IROp::FMul: emit("mulss\txmm0, xmm1"); break;
        case IROp::FDiv: emit("divss\txmm0, xmm1"); break;
        default: break;
    }
    
    auto it = regMap.find(instr.dest.id);
    if (it != regMap.end()) {
        emit("movq\t" + it->second + ", xmm0");
    } else {
        emit("movss\t" + fStackRef(instr.dest.id) + ", xmm0");
    }
}

void CodeGen::emitCall(const IRInstruction& instr) {
    static const char* intRegs[]   = { "rdi","rsi","rdx","rcx","r8","r9" };
    static const char* floatRegs[] = { "xmm0","xmm1","xmm2","xmm3",
                                       "xmm4","xmm5","xmm6","xmm7" };

    int intIdx = 0, floatIdx = 0;
    for (const IRValue& arg : instr.args) {
        if (arg.type == Type::Float32t) {
            emit("mov\trax, " + resolve(arg));
            emit("movq\t" + std::string(floatRegs[floatIdx++]) + ", rax");
        } else if (arg.type == Type::Stringt) {
            
            emit("lea\trax, [" + arg.label + "_len + rip]");
            emit("mov\t" + std::string(intRegs[intIdx++]) + ", QWORD PTR [rax]");
            emit("lea\trax, [" + arg.label + " + rip]");
            emit("mov\t" + std::string(intRegs[intIdx++]) + ", rax");
        } else {
            emit("mov\t" + std::string(intRegs[intIdx++]) + ", " + resolve(arg));
        }
    }

    emit("call\t" + instr.label);

    if (instr.dest.kind == IRValue::Kind::Temp) {
        auto it = regMap.find(instr.dest.id);
        if (it != regMap.end())
            emit("mov\t" + it->second + ", rax");
        else
            emit("mov\t" + stackRef(instr.dest.id) + ", rax");
    }
}

void CodeGen::emitCmp(const IRInstruction& instr, const std::string& setcc) {
    emit("mov\trax, " + resolve(instr.src1));
    emit("mov\tr9,  " + resolve(instr.src2));
    emit("cmp\trax, r9");
    emit(setcc + "\tal");
    emit("movzx\trax, al");
    emit("mov\t" + resolve(instr.dest) + ", rax");
}

void CodeGen::emitConst(const IRInstruction& instr) {
    std::string dest = resolve(instr.dest);
    emit("mov\trax, " + std::to_string(instr.src1.ival));
    emit("mov\t" + dest + ", rax");
}

void CodeGen::emitFConst(const IRInstruction& instr) {
    float fval = instr.src1.fval;
    std::string lbl = floatLabel(fval);
    emit("movss\txmm0, DWORD PTR [" + lbl + " + rip]");
    emit("movq\trax, xmm0");
    emit("mov\t" + resolve(instr.dest) + ", rax");
}

void CodeGen::emitIndex(const IRInstruction& instr) {
    emit("mov\trax, " + resolve(instr.src1));
    emit("mov\trcx, " + resolve(instr.src2));
    emit("movzx\teax, BYTE PTR [rax + rcx]");
    emit("mov\t" + resolve(instr.dest) + ", rax");
}

void CodeGen::emitNot(const IRInstruction& instr) {
    // !x == (x == 0)
    emit("mov\trax, " + resolve(instr.src1));
    emit("cmp\trax, 0");
    emit("sete\tal");
    emit("movzx\trax, al");
    emit("mov\t" + resolve(instr.dest) + ", rax");
}

void CodeGen::emitParam(const IRInstruction& instr) {
    static const char* intRegs[]   = { "rdi","rsi","rdx","rcx","r8","r9" };
    static const char* floatRegs[] = { "xmm0","xmm1","xmm2","xmm3",
                                       "xmm4","xmm5","xmm6","xmm7" };

    int paramIdx = instr.src1.ival;
    Type paramType = currentFunc->params[paramIdx].type;

    int intIdx = 0, floatIdx = 0;
    for (int i = 0; i < paramIdx; i++) {
        if (currentFunc->params[i].type == Type::Float32t) floatIdx++;
        else intIdx++;
    }

    if (paramType == Type::Float32t) {
        emit("movss\t" + fStackRef(instr.dest.id) + ", " + floatRegs[floatIdx]);
    } else {
        emit("mov\t" + resolve(instr.dest) + ", " + intRegs[intIdx]);
    }
}

void CodeGen::emitRet(const IRInstruction& instr) {
    if (currentFunc->returnType == Type::Float32t) {
        if (instr.src1.kind == IRValue::Kind::Temp) {
            emit("mov\trax, " + resolve(instr.src1));
        } else {
            std::string lbl = floatLabel(instr.src1.fval);
            emit("movss\txmm0, DWORD PTR [" + lbl + " + rip]");
            emitEpilogue();
            return;
        }
        emit("movq\txmm0, rax");
    } else {
        emit("mov\trax, " + resolve(instr.src1));
    }
    emitEpilogue();
}

void CodeGen::emitToFloat(const IRInstruction& instr) {
    // i32 → f32
    emit("mov\teax, DWORD PTR " + resolve(instr.src1));
    emit("cvtsi2ss\txmm0, eax");
    emit("movss\t" + fStackRef(instr.dest.id) + ", xmm0");
}

void CodeGen::emitToInt(const IRInstruction& instr) {
    if (instr.src1.type == Type::Float32t) {
        emit("mov\trax, " + resolve(instr.src1));
        emit("movq\txmm0, rax");
        emit("cvttss2si\teax, xmm0");
        emit("mov\t" + resolve(instr.dest) + ", rax");
    } else {
        emit("mov\trax, " + resolve(instr.src1));
        emit("movsxd\trax, eax");
        emit("mov\t" + resolve(instr.dest) + ", rax");
    }
}