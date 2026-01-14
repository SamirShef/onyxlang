#include <onyx/AST/Printer.h>
#include <onyx/CodeGen/CodeGen.h>
#include <onyx/Compilation/Compilation.h>
#include <onyx/Compilation/Optimizer.h>
#include <onyx/Compilation/Options.h>
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
    
    llvm::cl::HideUnrelatedOptions(onyx::OnyxCat);
    llvm::cl::ParseCommandLineOptions(argc, argv, "Onyx Compiler\n");

    std::string fileName = onyx::InputFilename;

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
    while (1) {
        onyx::Stmt *stmt = parser.ParseStmt();
        if (!stmt) {
            break;
        }
        ast.push_back(stmt);
    }
    if (diag.HasErrors()) {
        return 1;
    }
    diag.ResetErrors();

    onyx::SemanticAnalyzer sema(diag);
    for (auto &stmt : ast) {
        sema.Visit(stmt);
    }
    if (diag.HasErrors()) {
        return 1;
    }
    diag.ResetErrors();

    onyx::CodeGen codegen(fileName);
    for (auto &stmt : ast) {
        codegen.Visit(stmt);
    }
    if (diag.HasErrors()) {
        return 1;
    }
    std::unique_ptr<llvm::Module> mod = codegen.GetModule();
    onyx::InitializeLLVMTargets();
    
    std::string tripleStr = llvm::sys::getDefaultTargetTriple();
    llvm::Triple triple(tripleStr);
    llvm::StringRef stem = llvm::sys::path::stem(fileName);
    std::string outputName;
    if (!onyx::OutputFilename.empty()) {
        outputName = onyx::OutputFilename;
    }
    else {
        switch (onyx::EmitAction) {
            case onyx::EmitAST:
                outputName = "";
                break;
            case onyx::EmitLLVM:
                outputName = (stem + ".ll").str();
                break;
            case onyx::EmitObj:
                outputName = (stem + ".o").str();
                break;
            case onyx::EmitBinary: 
                outputName = stem.str();
                if (triple.isOSWindows()) {
                    outputName += ".exe";
                }
                break;
        }
    }

    if (onyx::EmitAction == onyx::EmitAST) {
        onyx::ASTPrinter printer;
        for (auto stmt : ast) {
            printer.Visit(stmt);
            llvm::outs() << '\n';
        }
        return 0; 
    }

    onyx::Optimizer::Level optLvl;
    switch (onyx::OptimizationLevel) {
        case onyx::O1:
            optLvl = onyx::Optimizer::O1;
            break;
        case onyx::O2:
            optLvl = onyx::Optimizer::O2;
            break;
        case onyx::O3:
            optLvl = onyx::Optimizer::O3;
            break;
        default:
            optLvl = onyx::Optimizer::O0;
            break;
    }
    if (onyx::OptimizationLevel > onyx::O0) {
        onyx::Optimizer optimizer;
        optimizer.Optimize(*mod, optLvl);
    }

    if (onyx::EmitAction == onyx::EmitLLVM) {
        std::error_code ec;
        llvm::raw_fd_ostream os(outputName, ec);
        if (ec) {
            llvm::errs() << ec.message();
            return 1;
        }
        mod->print(os, nullptr);
        return 0;
    }
    
    std::string objFile = (onyx::EmitAction == onyx::EmitObj) ? outputName : (stem + ".o").str();
    if (!onyx::EmitObjectFile(&*mod, objFile, tripleStr)) {
        return 1;
    }
    if (onyx::EmitAction == onyx::EmitBinary) {
        onyx::LinkObjectFile(objFile, onyx::GetOutputName(fileName, triple));
        llvm::sys::fs::remove(objFile);
    }
    llvm::outs().flush();   // explicitly flushing the buffer
    return 0;
}