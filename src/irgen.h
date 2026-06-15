#ifndef cuprum_irgen_h
#define cuprum_irgen_h

#include "common.h"
#include "ast.h"
#include "ir.h"
#include "symbol_table.h"

class IRGen : public ASTVisitor {
public:
    IRGen(SymbolTable* table) : table(table) { }
    IRProgram emit(ASTProgram* program); // entry point

    void visit(BlockStmt&)   override;
    void visit(ExprStmt&)    override;
    void visit(ForStmt&)     override;
    void visit(FuncDecl&)    override;
    void visit(IfStmt&)      override;
    void visit(LetStmt&)     override;
    void visit(ReturnStmt&)  override;
    void visit(WhileStmt&)   override;

    void visit(AssignExpr&)  override;
    void visit(BinaryExpr&)  override;
    void visit(CallExpr&)    override;
    void visit(CastExpr&)    override;
    void visit(IndexExpr&)   override;
    void visit(LiteralExpr&) override;
    void visit(UnaryExpr&)   override;
    void visit(VarExpr&)     override;

    std::unordered_map<std::string, std::string> getStrLabels() {
        return stringLabels;
    }

private:
    IRProgram  program;
    IRFunction currentFunc; // the function being built
    IRValue    lastValue;   // result of the most recently visited Expr
    int        tempCount = 0;
    int labelCount = 0;

    SymbolTable* table;
    ASTProgram* AST;

    std::unordered_map<std::string, int> varTemps;
    std::unordered_map<std::string, std::string> stringLabels;

    IRValue newTemp();  // returns IRValue{Temp, tempCount_++}
    std::string newLabel(const std::string& prefix);
    std::string stringLabel(const std::string& value);
    void    emit(IRInstruction); // appends to currentFunc.body
};

#endif