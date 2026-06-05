#ifndef ferrum_irgen_h
#define ferrum_irgen_h

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
    void visit(FuncDecl&)    override;
    void visit(IfStmt&)      override;
    void visit(LetStmt&)     override;
    void visit(ReturnStmt&)  override;

    void visit(AssignExpr&)  override;
    void visit(BinaryExpr&)  override;
    void visit(CastExpr&)    override;
    
    void visit(LiteralExpr&) override;
    void visit(UnaryExpr&)   override;
    void visit(VarExpr&)     override;

private:
    IRProgram  program;
    IRFunction currentFunc; // the function being built
    IRValue    lastValue;   // result of the most recently visited Expr
    int        tempCount = 0;
    int labelCount = 0;

    SymbolTable* table;
    ASTProgram* AST;

    std::unordered_map<std::string, int> varTemps;

    IRValue newTemp();  // returns IRValue{Temp, tempCount_++}
    std::string newLabel(const std::string& prefix);
    void    emit(IRInstruction); // appends to currentFunc_.body
};

#endif