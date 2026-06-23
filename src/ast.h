#ifndef cuprum_ast_h
#define cuprum_ast_h

#include <variant>
#include <vector>

#include "token.h"

enum class Type {
    Arrayt,
    Boolt,
    Int32t,
    Int64t,
    Float32t,
    Chart,
    Stringt,
    Structt,
    Voidt,
    Nullt,
};

struct ArrayType {
    Type elementType = Type::Nullt;
    int  size        = 0;
};

struct FieldDef {
    std::string name;
    Type        type;
    ArrayType   arrayType;  // if field is an array
    std::string structName; // if field is itself a struct
};

struct StructDef {
    std::string            name;
    std::vector<FieldDef>  fields;

    int offsetOf(const std::string& fieldName) const {
        for (int i = 0; i < (int)fields.size(); i++)
            if (fields[i].name == fieldName) return i * 8;
        return -1;
    }

    int size() const { return (int)fields.size() * 8; }
};

using StructRegistry = std::unordered_map<std::string, StructDef>;

inline std::string TypetoString(Type t) {
    switch(t) {
        case Type::Arrayt:   return "array";
        case Type::Boolt:    return "bool";
        case Type::Chart:    return "char";
        case Type::Int32t:   return "i32";
        case Type::Int64t:   return "i64";
        case Type::Float32t: return "f32";
        case Type::Stringt:  return "String";
        case Type::Structt:  return "struct";
        case Type::Voidt:    return "void";
        case Type::Nullt:    return "null";
        default: return "?";
    }
}

class ArrayLiteral;
class AssignExpr;
class BinaryExpr;
class CallExpr;
class CastExpr;
class FieldExpr;
class FieldAssignExpr;
class IndexExpr;
class IndexAssignExpr;
class LiteralExpr;
class StructLiteral;
class UnaryExpr;
class VarExpr;

class BlockStmt;
class BreakStmt;
class ContinueStmt;
class ExprStmt;
class ForStmt;
class FuncDecl;
class IfStmt;
class NativeStmt;
class LetStmt;
class ReturnStmt;
class WhileStmt;

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    virtual void visit(ArrayLiteral&) = 0;
    virtual void visit(AssignExpr&)   = 0;
    virtual void visit(BinaryExpr&)   = 0;
    virtual void visit(CallExpr&)     = 0;
    virtual void visit(CastExpr&)     = 0;
    virtual void visit(FieldExpr&)    = 0;
    virtual void visit(FieldAssignExpr&) = 0;
    virtual void visit(IndexExpr&)    = 0;
    virtual void visit(IndexAssignExpr&) = 0;
    virtual void visit(LiteralExpr&)  = 0;
    virtual void visit(StructLiteral&)= 0;
    virtual void visit(UnaryExpr&)    = 0;
    virtual void visit(VarExpr&)      = 0;

    virtual void visit(BlockStmt&)    = 0;
    virtual void visit(BreakStmt&)    = 0;
    virtual void visit(ContinueStmt&) = 0;
    virtual void visit(ExprStmt&)     = 0;
    virtual void visit(ForStmt&)      = 0;
    virtual void visit(FuncDecl&)     = 0;
    virtual void visit(IfStmt&)       = 0;
    virtual void visit(NativeStmt&)   = 0;
    virtual void visit(LetStmt&)      = 0;
    virtual void visit(ReturnStmt&)   = 0;
    virtual void visit(WhileStmt&)    = 0;
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

class ArrayLiteral : public Expr {
public:
    void accept(ASTVisitor& v) override { 
        v.visit(*this); 
    }

    std::vector<Expr*> elements;
    ArrayType          arrayType; // filled by sema
    Token              bracket;
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

class FieldExpr : public Expr {
public:
    void accept(ASTVisitor& v) override { 
        v.visit(*this); 
    }

    Expr*  object;
    Token  field;
    std::string structName;
};

class FieldAssignExpr : public Expr {
public:
    void accept(ASTVisitor& v) override { v.visit(*this); }

    Expr*  object;
    Token  field;
    Expr*  value;
};

class IndexExpr : public Expr {
public:
    void accept(ASTVisitor& v) override { 
        v.visit(*this); 
    }

    Expr*  object;
    Expr*  index;
    Token  bracket;
    Type   elemType = Type::Nullt;
};

class IndexAssignExpr : public Expr {
public:
    void accept(ASTVisitor& v) override { 
        v.visit(*this); 
    }

    Expr*  object;
    Expr*  index;
    Expr*  value;
    Token  bracket;
};

class LiteralExpr : public Expr {
public:
    void accept(class ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    std::variant<int32_t, int64_t, _Float32, bool, char, std::string> value;
};

class StructLiteral : public Expr {
public:
    void accept(ASTVisitor& v) override { 
        v.visit(*this); 
    }

    std::vector<Expr*> fields;
    Token              brace;
    std::string        structName;
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

class BreakStmt : public Stmt {
public:
    void accept(ASTVisitor& v) override { 
        v.visit(*this); 
    }

    Token token;
};

class ContinueStmt : public Stmt {
public:
    void accept(ASTVisitor& v) override { 
        v.visit(*this); 
    }

    Token token;
};

class ExprStmt : public Stmt {
public:
    void accept(class ASTVisitor& visitor) override {
        visitor.visit(*this);
    }

    Expr* expression;
};

struct VarDeclarator {
    Token      name;
    Type       type      = Type::Nullt;
    ArrayType  arrayType;
    std::string structName;
    bool       isConst   = false;
    bool       inferred  = false;
    Expr*      init      = nullptr;
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
    Token       name;
    Type        type;
    ArrayType   arrayType;
    std::string structName;
};

class FuncDecl : public Stmt {
public:
    void accept(ASTVisitor& v) override { v.visit(*this); }

    Type             returnType;
    ArrayType        returnArrayType;
    Token            name;
    std::vector<Param> params;
    BlockStmt*       body;
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

class LetStmt : public Stmt {
public:
    void accept(ASTVisitor& v) override { 
        v.visit(*this); 
    }

    std::vector<VarDeclarator> declarators;
};

class NativeStmt : public Stmt {
public:
    void accept(ASTVisitor& v) override {
        v.visit(*this);
    }

    Type returnType;
    Token name;
    std::vector<Param> params;
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

class StructDecl {
public:
    Token              name;
    std::vector<FieldDef> fields;
};

struct ASTProgram {
    std::vector<FuncDecl*> functions;
    FuncDecl* mainFunction = nullptr;

    std::vector<NativeStmt*> natives;
    std::vector<StructDecl*> structs;
};

#endif
