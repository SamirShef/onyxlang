#include "onyx/parser/Parser.h"
#include <llvm/Support/raw_ostream.h>
#include <onyx/lexer/Lexer.h>

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

    onyx::Stmt *stmt = parser.ParseStmt();
    while (stmt) {
        stmt = parser.ParseStmt();
    }
    return 0;
}