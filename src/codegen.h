#ifndef cuprum_codegen_h
#define cuprum_codegen_h

#include <fstream>
#include <unordered_map>
#include "common.h"
#include "ir.h"

// Tracks each temp's stack slot (offset from rbp, e.g. -8, -16, ...)
class StackFrame {
public:
    // Assigns the next available slot to tempId. Returns the offset.
    int allocate(int tempId) {
        if (offsets.contains(tempId)) {
            return offsets.at(tempId);
        }

        int offset = nextOffset;
        offsets[tempId] = offset;
        nextOffset += 8;
        return offset;
    }

    // Returns the rbp offset for a previously allocated temp.
    int offsetOf(int tempId) const {
        return offsets.at(tempId);
    }

    // Total bytes needed — rounded up to 16-byte alignment.
    int frameSize() const {
        int size = nextOffset - 8; // nextOffset starts at 8
        return (size + 15) & ~15;   // round up to nearest 16
    }

    bool hasSlot(int tempId) const {
        return offsets.count(tempId) > 0;
    }

    void reset() {
        offsets.clear();
        nextOffset = 8;
    }

private:
    std::unordered_map<int, int> offsets;
    int nextOffset = 8; // first slot is [rbp-8]
};

class CodeGen {
public:
    CodeGen(IRProgram& program, std::unordered_map<std::string, std::string> strLabels) 
        : program(program), stringLabels(strLabels) {}

    // Entry point. Writes the full .s file to outputPath.
    void generate(const std::string& outputPath);

private:
    IRProgram& program;
    const IRFunction* currentFunc;
    std::ofstream out;
    StackFrame frame;

    std::unordered_map<float, std::string> floatLabels;
    std::unordered_map<std::string, std::string> stringLabels;
    std::vector<int> paramTempIds;

    void emitPreamble();

    void emitFunction(const IRFunction& fn);

    void emitPrologue(const IRFunction& fn, int frameSize);

    void emitEpilogue();

    // Dispatches a single instruction.
    void emitInstruction(const IRInstruction& instr);

    void emitBinop  (const IRInstruction& instr); // dest = src1 op src2
    void emitFBinop (const IRInstruction& instr);
    void emitCall   (const IRInstruction& instr);
    void emitCmp    (const IRInstruction& instr, const std::string& setcc);
    void emitConst  (const IRInstruction& instr); // dest = constant
    void emitFConst (const IRInstruction& instr);
    void emitIndex  (const IRInstruction& instr);
    void emitNot    (const IRInstruction& instr);
    void emitParam  (const IRInstruction& instr);
    void emitRet    (const IRInstruction& instr); // return src1
    void emitToFloat(const IRInstruction& instr);
    void emitToInt  (const IRInstruction& instr);

    std::string stackRef (int tempId) const;
    std::string fStackRef(int tempId) const;

    std::string resolve(const IRValue& val) const;

    void emit(const std::string& line);

    void emitLabel(const std::string& label);

    std::string floatLabel(float fval);
    void emitFloatPool();

    void emitStringPool();
};

#endif