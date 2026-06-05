#include "codegen.h"

void CodeGen::emit(const std::string& line) {
    out_ << "\t" << line << "\n";
}

void CodeGen::emitLabel(const std::string& label) {
    out_ << label << ":\n";
}

std::string CodeGen::stackRef(int tempId) const {
    return "QWORD PTR [rbp-" + std::to_string(frame_.offsetOf(tempId)) + "]";
}

std::string CodeGen::resolve(const IRValue& val) const {
    if (val.kind == IRValue::Kind::Temp)
        return stackRef(val.id);
    return std::to_string(val.constant);
}


void CodeGen::generate(const std::string& outputPath) {
    out_.open(outputPath);
    emitPreamble();
    for (const IRFunction& fn : program_.functions)
        emitFunction(fn);
    out_.close();
}

void CodeGen::emitPreamble() {
    out_ << ".intel_syntax noprefix\n\n";

    // _start: call main, pass its return value to exit syscall
    out_ << ".global _start\n";
    emitLabel("_start");
    emit("call\tmain");
    emit("mov\trdi, rax");   // exit code = return value of main
    emit("mov\trax, 60");    // syscall number: exit
    emit("syscall");
    out_ << "\n";
}


void CodeGen::emitFunction(const IRFunction& fn) {
    frame_.reset();

    // Dry-run: allocate every temp
    for (const IRInstruction& instr : fn.body) {
        if (instr.dest.kind == IRValue::Kind::Temp)
            frame_.allocate(instr.dest.id);
    }

    out_ << ".global " << fn.name << "\n";
    emitLabel(fn.name);
    emitPrologue(frame_.frameSize());

    for (const IRInstruction& instr : fn.body)
        emitInstruction(instr);
}

void CodeGen::emitPrologue(int frameSize) {
    emit("push\trbp");
    emit("mov\trbp, rsp");
    if (frameSize > 0)
        emit("sub\trsp, " + std::to_string(frameSize));
}

void CodeGen::emitEpilogue() {
    emit("mov\trsp, rbp");
    emit("pop\trbp");
    emit("ret");
}


void CodeGen::emitInstruction(const IRInstruction& instr) {
    switch (instr.op) {
        case IROp::Const:  emitConst(instr);  break;
        case IROp::Ret:    emitRet(instr);    break;
        case IROp::Add:
        case IROp::Sub:
        case IROp::Mul:
        case IROp::Div:    emitBinop(instr);  break;
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
        case IROp::Call:
            emit("call\t" + instr.label);
            if (instr.dest.kind == IRValue::Kind::Temp)
                emit("mov\t" + stackRef(instr.dest.id) + ", rax");
            break;
        case IROp::Mov:
            emit("mov\trax, " + resolve(instr.src1));
            emit("mov\t" + stackRef(instr.dest.id) + ", rax");
            break;
        case IROp::ToBool:
            emit("mov\trax, " + resolve(instr.src1));
            emit("cmp\trax, 0");
            emit("setne\tal");
            emit("movzx\trax, al");
            emit("mov\t" + stackRef(instr.dest.id) + ", rax");
            break;
    }
}

//   mov rax, <value>
//   mov [rbp-N], rax
void CodeGen::emitConst(const IRInstruction& instr) {
    if (instr.src1.constant == 0) {
        emit("xor\teax, eax");
    } else {
        emit("mov\trax, " + std::to_string(instr.src1.constant));
    }

    emit("mov\t" + stackRef(instr.dest.id) + ", rax");
}

void CodeGen::emitCmp(const IRInstruction& instr, const std::string& setcc) {
    emit("mov\trax, " + resolve(instr.src1));
    emit("mov\trbx, " + resolve(instr.src2));
    emit("cmp\trax, rbx");
    emit(setcc + "\tal");
    emit("movzx\trax, al");
    emit("mov\t" + stackRef(instr.dest.id) + ", rax");
}

void CodeGen::emitNot(const IRInstruction& instr) {
    // !x == (x == 0)
    emit("mov\trax, " + resolve(instr.src1));
    emit("cmp\trax, 0");
    emit("sete\tal");
    emit("movzx\trax, al");
    emit("mov\t" + stackRef(instr.dest.id) + ", rax");
}

//   ret src1
//   mov rax, [rbp-N]   (return value goes in rax per System V ABI)
//   epilogue
void CodeGen::emitRet(const IRInstruction& instr) {
    if (instr.src1.kind == IRValue::Kind::Temp)
        emit("mov\trax, " + stackRef(instr.src1.id));
    else
        emit("mov\trax, " + std::to_string(instr.src1.constant));
    emitEpilogue();
}

// dest = src1 op src2
//   mov rax, src1
//   mov rbx, src2
//   <op> rax, rbx
//   mov [rbp-N], rax
void CodeGen::emitBinop(const IRInstruction& instr) {
    // Load src1 into rax
    if (instr.src1.kind == IRValue::Kind::Temp)
        emit("mov\trax, " + stackRef(instr.src1.id));
    else
        emit("mov\trax, " + std::to_string(instr.src1.constant));

    // Load src2 into rbx
    if (instr.src2.kind == IRValue::Kind::Temp)
        emit("mov\trbx, " + stackRef(instr.src2.id));
    else
        emit("mov\trbx, " + std::to_string(instr.src2.constant));

    switch (instr.op) {
        case IROp::Add: emit("add\trax, rbx"); break;
        case IROp::Sub: emit("sub\trax, rbx"); break;
        case IROp::Mul: emit("imul\trax, rbx"); break;
        case IROp::Div:
            // idiv uses rdx:rax as the dividend; cqo sign-extends rax into rdx
            emit("cqo");
            emit("idiv\trbx");
            // quotient is in rax, remainder in rdx
            break;
        default: break;
    }

    emit("mov\t" + stackRef(instr.dest.id) + ", rax");
}