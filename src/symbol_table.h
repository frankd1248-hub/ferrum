#ifndef cuprum_symbol_table_h
#define cuprum_symbol_table_h

#include "common.h"
#include "token.h"
#include "ast.h"

struct Symbol {
    std::string name;
    Type        type;
    ArrayType   arrayType;
    std::string structName;
    bool        isFunction  = false;
    bool        isConst     = false;
    std::vector<Type> paramTypes;
    Token       declToken;
};

class SymbolTable {
public:
    SymbolTable() : scopes({}) { }

    void pushScope() {
        scopes.push_back({});
    }

    void popScope() {
        scopes.pop_back();
    }

    bool define(const Symbol&);     // false if already defined in current scope
    Symbol* lookup(const std::string& name); // walks stack outward, nullptr if not found
private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes;
};

#endif