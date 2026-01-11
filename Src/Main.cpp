#include <onyx/AST/Printer.h>
#include <onyx/CodeGen/CodeGen.h>
#include <onyx/Compilation/Compilation.h>
#include <onyx/Lexer/Lexer.h>
#include <onyx/Parser/Parser.h>
#include <onyx/Sema/Semantic.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/TargetParser/Host.h>

int
main(int argc, char **argv) {
    if (argc < 2) {
        llvm::errs() << llvm::errs().RED << "Usage: onyxc <input file>\n" << llvm::errs().RESET;
        return 1;
    }
    
    std::string fileName = argv[1];

    llvm::SourceMgr srcMgr;
    auto bufferOrErr = llvm::MemoryBuffer::getFile(fileName);
    
    if (std::error_code ec = bufferOrErr.getError()) {
        llvm::errs() << llvm::errs().RED << "Could not open file: " << llvm::errs().RESET << ec.message() << '\n';
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
    if (diag.HasErrors()) {
        return 1;
    }
    diag.ResetErrors();

    onyx::SemanticAnalyzer sema(diag);
    for (auto &stmt : ast) {
        sema.visit(stmt);
    }
    if (diag.HasErrors()) {
        return 1;
    }
    diag.ResetErrors();

    onyx::CodeGen codegen(diag, fileName);
    for (auto &stmt : ast) {
        codegen.visit(stmt);
    }
    if (diag.HasErrors()) {
        return 1;
    }
    std::unique_ptr<llvm::Module> mod = codegen.GetModule();
    onyx::InitializeLLVMTargets();
    std::string tripleStr = llvm::sys::getDefaultTargetTriple();
    llvm::Triple triple(tripleStr);
    std::string objFile = llvm::sys::path::stem(fileName).str() + ".o";
    std::string exeFile = onyx::GetOutputName(fileName, triple);
    if (onyx::EmitObjectFile(&*mod, objFile, tripleStr)) {
        llvm::outs() << llvm::outs().CYAN << "Compilation successful! Object file written to: " << llvm::outs().RESET << objFile << '\n';
    }
    else {
        return 1;
    }
    onyx::LinkObjectFile(objFile, onyx::GetOutputName(fileName, triple));
    llvm::sys::fs::remove(objFile);
    llvm::outs().flush();   // explicitly flushing the buffer
    return 0;
}