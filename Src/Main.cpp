#include <onyx/AST/Printer.h>
#include <onyx/Lexer/Lexer.h>
#include <onyx/Parser/Parser.h>
#include <onyx/Sema/Semantic.h>
#include <llvm/Support/raw_ostream.h>

int
main(int argc, char **argv) {
    if (argc < 2) {
        llvm::errs() << "Usage: onyxc <input file>\n";
        return 1;
    }
    
    std::string filename = argv[1];

    llvm::SourceMgr srcMgr;
    auto bufferOrErr = llvm::MemoryBuffer::getFile(filename);
    
    if (std::error_code ec = bufferOrErr.getError()) {
        llvm::errs() << "Could not open file: " << ec.message() << '\n';
        return 1;
    }
    srcMgr.AddNewSourceBuffer(std::move(*bufferOrErr), llvm::SMLoc());

    onyx::DiagnosticEngine diag(srcMgr);
    
    onyx::Lexer lex(srcMgr, diag);
    onyx::Parser parser(lex, diag);

    std::vector<onyx::Stmt *> ast;
    
    onyx::ASTPrinter printer;
    while (1) {
        onyx::Stmt *stmt = parser.ParseStmt();
        if (!stmt) {
            break;
        }
        printer.visit(stmt);
        ast.push_back(stmt);
        llvm::outs() << '\n';
    }

    onyx::SemanticAnalyzer sema(diag);
    for (auto &stmt : ast) {
        sema.visit(stmt);
    }
    llvm::outs().flush();   // explicitly flushing the buffer
    return 0;
}