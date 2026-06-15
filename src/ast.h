#ifndef cuprum_ast_h
#define cuprum_ast_h

#include <variant>
#include <vector>

#include "token.h"

enum class Type {
    Boolt,
    Int32t,
    Float32t,
    Chart,
    Stringt,
    Voidt,
    Nullt,
};

inline std::string TypetoString(Type t) {
    switch(t) {
        case Type::Boolt:    return "bool";
        case Type::Chart:    return "char";
        case Type::Int32t:   return "i32";
        case Type::Float32t: return "f32";
        case Type::Stringt:  return "String";
        case Type::Voidt:    return "void";
        case Type::Nullt:    return "null";
        default: return "?";
    }
}

class AssignExpr;
class BinaryExpr;
class CallExpr;
class CastExpr;
class IndexExpr;
class LiteralExpr;
class UnaryExpr;
class VarExpr;

class BlockStmt;
class ExprStmt;
class ForStmt;
class FuncDecl;
class IfStmt;
class LetStmt;
class ReturnStmt;
class WhileStmt;

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual void visit(AssignExpr&)  = 0;
    virtual void visit(BinaryExpr&)  = 0;
    virtual void visit(CallExpr&)    = 0;
    virtual void visit(CastExpr&)    = 0;
    virtual void visit(IndexExpr&)   = 0;
    virtual void visit(LiteralExpr&) = 0;
    virtual void visit(UnaryExpr&)   = 0;
    virtual void visit(VarExpr&)     = 0;

    virtual void visit(BlockStmt&)   = 0;
    virtual void visit(ExprStmt&)    = 0;
    virtual void visit(ForStmt&)     = 0;
    virtual void visit(FuncDecl&)    = 0;
    virtual void visit(IfStmt&)      = 0;
    virtual void visit(LetStmt&)     = 0;
    virtual void visit(ReturnStmt&)  = 0;
    virtual void visit(WhileStmt&)   = 0;
};

class ASTNode {

public:
    virtual ~ASTNode() = default;
    virtual void accept(class ASTVisitor& visitor) = 0;
};

class Expr : public ASTNode {
public:
    Type resolvedType;
};

class AssignExpr : public Expr {
public:
    void accept(ASTVisitor& v) override { 
        v.visit(*this); 
    }

    Token  name;
    Expr*  value;
};

class BinaryExpr : public Expr {
public:
    void accept(class ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    Expr* left;
    Expr* right;
    Token op;
};

class CallExpr : public Expr {
public:
    void accept(ASTVisitor& v) override { v.visit(*this); }

    Token              callee;
    std::vector<Expr*> args;
};

class CastExpr : public Expr {
public:
    void accept(ASTVisitor& v) override { 
        v.visit(*this); 
    }

    Type   targetType;
    Expr*  expr;
    Token  token;
};

class IndexExpr : public Expr {
public:
    void accept(ASTVisitor& v) override { 
        v.visit(*this); 
    }

    Expr*  object;
    Expr*  index;
    Token  bracket;
};

class LiteralExpr : public Expr {
public:
    void accept(class ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::variant<int32_t, _Float32, bool, char, std::string> value;
};

class UnaryExpr : public Expr {
public:
    void accept(class ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    Expr* right;
    Token op;
};

class VarExpr : public Expr {
public:
    void accept(ASTVisitor& v) override { 
        v.visit(*this); 
    }

    Token name;
};

class Stmt : public ASTNode {

};

class BlockStmt : public Stmt {
public:
    void accept(class ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::vector<Stmt*> stmts;
};

class ExprStmt : public Stmt {
public:
    void accept(class ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    Expr* expression;
};

struct VarDeclarator {
    Token   name;
    Type    type;
    bool    isConst = false;
    Expr*   init;
};

class ForStmt : public Stmt {
public:
    void accept(class ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    Token    start; // For error messages
    LetStmt* init;
    Expr*    condition;
    Expr*    increment;
    Stmt*    body;
};

struct Param {
    Token name;
    Type  type;
};

class FuncDecl : public Stmt {
public:
    void accept(ASTVisitor& v) override { v.visit(*this); }

    Type             returnType;
    Token            name;
    std::vector<Param> params;
    BlockStmt*       body;
};

class LetStmt : public Stmt {
public:
    void accept(ASTVisitor& v) override { 
        v.visit(*this); 
    }

    std::vector<VarDeclarator> declarators;
};

class IfStmt : public Stmt {
public:
    void accept(ASTVisitor& v) override {
        v.visit(*this);
    }

    Token start;
    Expr* condition;
    Stmt* thenBranch;
    Stmt* elseBranch = nullptr;
};

class ReturnStmt : public Stmt {
public:
    void accept(class ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    Expr* expr;
};

class WhileStmt : public Stmt {
public:
    void accept(class ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    Token start;
    Expr* condition;
    Stmt* body;
};

struct ASTProgram {
    std::vector<FuncDecl*> functions;
    FuncDecl* mainFunction = nullptr;
};

#endif
