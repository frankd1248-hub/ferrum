#ifndef cuprum_irgen_h
#define cuprum_irgen_h

#include "common.h"
#include "ast.h"
#include "ir.h"
#include "symbol_table.h"

struct LoopLabels {
    std::string condLabel; // continue jumps here
    std::string endLabel;  // break jumps here
};

struct ArrayInfo {
    Type elementType;
    int  size;
};

class IRGen : public ASTVisitor {
public:
    IRGen(SymbolTable* table, StructRegistry* reg) 
        : table(table), registry(reg) { }
    IRProgram emit(ASTProgram* program); // entry point

    void visit(BlockStmt&)    override;
    void visit(BreakStmt&)    override;
    void visit(ContinueStmt&) override;
    void visit(ExprStmt&)     override;
    void visit(ForStmt&)      override;
    void visit(FuncDecl&)     override;
    void visit(IfStmt&)       override;
    void visit(LetStmt&)      override;
    void visit(NativeStmt&)   override;
    void visit(ReturnStmt&)   override;
    void visit(WhileStmt&)    override;

    void visit(ArrayLiteral&) override;
    void visit(AssignExpr&)   override;
    void visit(BinaryExpr&)   override;
    void visit(CallExpr&)     override;
    void visit(CastExpr&)     override;
    void visit(FieldExpr&)    override;
    void visit(FieldAssignExpr&) override;
    void visit(IndexExpr&)    override;
    void visit(IndexAssignExpr&) override;
    void visit(LiteralExpr&)  override;
    void visit(StructLiteral&) override;
    void visit(UnaryExpr&)    override;
    void visit(VarExpr&)      override;

    std::unordered_map<std::string, std::string> getStrLabels() {
        return stringLabels;
    }

private:
    IRProgram  program;
    IRFunction currentFunc; // the function being built
    IRValue    lastValue;   // result of the most recently visited Expr
    int        tempCount = 0;
    int labelCount = 0;
    std::string lastStringLabel;

    SymbolTable* table;
    StructRegistry* registry;
    ASTProgram* AST;

    std::unordered_map<std::string, int> varTemps;
    std::unordered_map<std::string, std::string> stringLabels;
    std::vector<LoopLabels> loopStack = {};
    std::unordered_map<std::string, ArrayInfo> varArrayInfo;
    std::unordered_map<std::string, std::string> varStructNames;

    IRValue newTemp();  // returns IRValue{Temp, tempCount_++}
    std::string newLabel(const std::string& prefix);
    std::string stringLabel(const std::string& value);
    void    emit(IRInstruction); // appends to currentFunc.body
};

#endif