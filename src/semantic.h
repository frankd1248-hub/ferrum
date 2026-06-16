#ifndef cuprum_semantic_h
#define cuprum_semantic_h

#include "common.h"
#include "ast.h"
#include "error_reporter.h"
#include "symbol_table.h"

class SemanticAnalyzer : public ASTVisitor {
public:
    SemanticAnalyzer(SymbolTable& sym, ErrorReporter& err) : symbols(sym), err(err) { }

    void analyze(ASTProgram& program); // entry point; calls accept() on the root

    void visit(BlockStmt& node)    override;
    void visit(BreakStmt& node)    override;
    void visit(ContinueStmt& node) override;
    void visit(ExprStmt& node)     override;
    void visit(ForStmt& node)      override;
    void visit(FuncDecl& node)     override;
    void visit(IfStmt& node)       override;
    void visit(NativeStmt& node)   override;
    void visit(LetStmt& node)      override;
    void visit(ReturnStmt& node)   override;
    void visit(WhileStmt& node)    override; 

    void visit(AssignExpr& node)   override;
    void visit(BinaryExpr& node)   override;
    void visit(CallExpr& node)     override;
    void visit(CastExpr& node)     override;
    void visit(IndexExpr& node)    override;
    void visit(LiteralExpr& node)  override;
    void visit(UnaryExpr& node)    override;
    void visit(VarExpr& node)      override;

private:
    SymbolTable&   symbols;
    ErrorReporter& err;
    Type currentReturnType = Type::Voidt; // set when entering a FuncDecl, checked in ReturnStmt
    int functionLinestart;
    int loopDepth = 0;
};

#endif