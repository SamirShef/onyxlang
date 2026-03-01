#include <filesystem>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <marble/Basic/Module.h>
#include <marble/Basic/ModuleManager.h>
#include <marble/AST/Printer.h>
#include <marble/CodeGen/CodeGen.h>
#include <marble/Compilation/Compilation.h>
#include <marble/Compilation/Optimizer.h>
#include <marble/Compilation/Options.h>
#include <marble/Lexer/Lexer.h>
#include <marble/Parser/Parser.h>
#include <marble/Sema/Semantic.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/TargetParser/Host.h>

int
main(int argc, char **argv) {
    if (argc < 2) {
        llvm::errs() << llvm::errs().RED << "Usage: marblec <input file>\n" << llvm::errs().RESET;
        return 1;
    }
    
    llvm::cl::HideUnrelatedOptions(marble::MarbleCat);
    llvm::cl::ParseCommandLineOptions(argc, argv, "Marble Compiler\n");
    llvm::outs().SetUnbuffered();

    std::string fileName = marble::InputFilename;

    llvm::SourceMgr srcMgr;
    marble::DiagnosticEngine diag(srcMgr);
    marble::Module *root = marble::ModuleManager::LoadModule(fileName, srcMgr, diag);
    if (!root) {
        llvm::errs() << llvm::errs().RED << "Could not open file: file not found" << llvm::errs().RESET << '\n';
        return 1;
    }

    if (marble::EmitAction == marble::EmitAST) {
        marble::ASTPrinter printer;
        for (auto stmt : root->AST) {
            printer.Visit(stmt);
            llvm::outs() << '\n';
        }
        return 0; 
    }

    std::string absoluteFileName = std::filesystem::absolute(fileName);
    std::string parentDir = absoluteFileName.substr(0, absoluteFileName.find_last_of("/\\"));
    marble::SemanticAnalyzer sema(parentDir, srcMgr, diag, root);
    sema.AnalyzeModule(root, true);

    if (root->Functions.find("main") == root->Functions.end()) {
        diag.Report(llvm::SMLoc::getFromPointer(srcMgr.getMemoryBuffer(srcMgr.getMainFileID())->getBufferStart()), marble::ErrDoesNotHaveMain);
    }

    if (diag.HasErrors()) {
        return 1;
    }
    diag.ResetErrors();

    marble::CodeGen codegen(parentDir, fileName, srcMgr, diag);
    codegen.DeclareMod(root);
    for (auto &stmt : root->AST) {
        codegen.Visit(stmt);
    }
    if (diag.HasErrors()) {
        return 1;
    }
    std::unique_ptr<llvm::Module> mod = codegen.GetModule();
    marble::InitializeLLVMTargets();
    
    std::string tripleStr = llvm::sys::getDefaultTargetTriple();
    llvm::Triple triple(tripleStr);
    int last_dot_pos = fileName.find_last_of('.');
    if (last_dot_pos != std::string::npos && last_dot_pos != 0) { 
        fileName.erase(last_dot_pos);
    }
    std::string outputName;
    if (!marble::OutputFilename.empty()) {
        outputName = marble::OutputFilename;
    }
    else {
        switch (marble::EmitAction) {
            case marble::EmitLLVM:
                outputName = fileName + ".ll";
                break;
            case marble::EmitObj:
                outputName = fileName + ".o";
                break;
            case marble::EmitBinary: 
                outputName = fileName;
                if (triple.isOSWindows()) {
                    outputName += ".exe";
                }
                break;
        }
    }

    marble::Optimizer::Level optLvl;
    switch (marble::OptimizationLevel) {
        case marble::O1:
            optLvl = marble::Optimizer::O1;
            break;
        case marble::O2:
            optLvl = marble::Optimizer::O2;
            break;
        case marble::O3:
            optLvl = marble::Optimizer::O3;
            break;
        default:
            optLvl = marble::Optimizer::O0;
            break;
    }
    if (marble::OptimizationLevel > marble::O0) {
        marble::Optimizer optimizer;
        optimizer.Optimize(*mod, optLvl);
    }

    if (marble::EmitAction == marble::EmitLLVM) {
        std::error_code ec;
        llvm::raw_fd_ostream os(outputName, ec);
        if (ec) {
            llvm::errs() << ec.message();
            return 1;
        }
        mod->print(os, nullptr);
        return 0;
    }
    
    std::string objFile = (marble::EmitAction == marble::EmitObj) ? outputName : fileName + ".o";
    if (!marble::EmitObjectFile(mod.get(), objFile, tripleStr)) {
        return 1;
    }
    if (marble::EmitAction == marble::EmitBinary) {
        marble::LinkObjectFile(objFile, outputName);
        llvm::sys::fs::remove(objFile); // NOLINT
    }
    llvm::outs().flush();   // explicitly flushing the buffer
    return 0;
}
