#include <sysexits.h>

#include "common.h"
#include "codegen.h"
#include "error_reporter.h"
#include "irgen.h"
#include "lexer.h"
#include "optimizer.h"
#include "parser.h"
#include "semantic.h"
#include "symbol_table.h"

using namespace std;

string readFile(const string& filename) {
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        exit(EX_IOERR);
    }

    ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char** argv) {
    if (argc != 2) {
        cerr << "Usage: cuprum <source_file>" << endl;
        exit(EX_USAGE);
    }

    string src = readFile(argv[1]);
    ErrorReporter err(cerr);
    Lexer lexer(src, err);
    vector<Token> tokens = lexer.tokenize();
    if (err.errored()) {
        cerr << "Lexer error.\n";
        exit(1);
    }

    // printTokens(cout, tokens);
    // cout << "\n\n";

    Parser parser(tokens, err);
    ASTProgram ast = parser.parse();

    if (parser.hadError) {
        std::cerr << "Parse errors found. Aborting.\n";
        return 1;
    }
    
    SymbolTable symtab;
    StructRegistry reg;
    SemanticAnalyzer sema(symtab, err, reg);
    sema.analyze(ast);
    if (err.errored()) {
        cerr << "Semantic error.\n";
        exit(1);
    }

    IRGen irgen(&symtab, &reg);
    IRProgram ir = irgen.emit(&ast);
    auto strlabels = irgen.getStrLabels();

    Optimizer optimizer(ir);
    optimizer.optimize();

    printIRProgram(ir);
    cout << "\n\n";

    string path = string(argv[1]);
    path = path.substr(path.find_last_of("/") + 1);
    path = path.substr(0, path.find_first_of("."));
    
    CodeGen codegen(ir, strlabels);
    codegen.generate(string("./build/") + path + ".s");

    string assembleInstruction = string("as ./build/") + path + ".s -o ./build/" + path + ".o"; 
    string linkInstruction = "ld -o " + path + " ./build/" + path + ".o ./stdlib/std.o";
    string deleteFiles = string("rm ") + path + ".s " + path + ".o";

    // cout << assembleInstruction << "\n" << linkInstruction << "\n" << deleteFiles << "\n";

    system(assembleInstruction.c_str());
    system(linkInstruction.c_str());
    // system(deleteFiles.c_str());

    return EX_OK;
};