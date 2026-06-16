#ifndef cuprum_optimizer_h
#define cuprum_optimizer_h

#include <vector>
#include "ir.h"

class Optimizer {
public:
    Optimizer(IRProgram& program) : program(program) {}
    void optimize();

private:
    IRProgram& program;

    void optimizeFunction(IRFunction& fn);

    void constantFolding(std::vector<IRInstruction>& body);
    void deadCodeElimination(std::vector<IRInstruction>& body);
    void copyPropagation(std::vector<IRInstruction>& body);
    void deadBranchElimination(std::vector<IRInstruction>& body);
    void unreachableCodeElimination(std::vector<IRInstruction>& body);
    void emptyLabelElimination(std::vector<IRInstruction>& body);
    void strengthReduction(std::vector<IRInstruction>& body);
    void redundantJumpElimination(std::vector<IRInstruction>& body);
};

#endif