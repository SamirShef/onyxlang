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

std::string libsPath = "Libs/";

int
main(int argc, char **argv) {
    if (argc < 2) {
        llvm::errs() << llvm::errs().RED << "Usage: marblec <input file>\n" << llvm::errs().RESET;
        return 1;
    }
    
    llvm::cl::HideUnrelatedOptions(marble::MarbleCat);
    llvm::cl::ParseCommandLineOptions(argc, argv, "Marble Compiler\n");
    llvm::outs().SetUnbuffered();

    llvm::SourceMgr srcMgr;
    std::string fileName = marble::InputFilename;

    auto bufferOrErr = llvm::MemoryBuffer::getFile(fileName);
    
    if (std::error_code ec = bufferOrErr.getError()) {
        llvm::errs() << llvm::errs().RED << "Could not open file " << llvm::errs().RESET << '`' << fileName << "`: " << ec.message() << '\n';
        return 1;
    }
    srcMgr.AddNewSourceBuffer(std::move(*bufferOrErr), llvm::SMLoc());

    marble::DiagnosticEngine diag(srcMgr);
    marble::ModuleManager modManager(diag);
    marble::Module *mainMod = modManager.LoadModule(fileName, marble::AccessPriv, srcMgr);
    if (diag.HasErrors()) {
        return 1;
    }
    diag.ResetErrors();

    if (marble::EmitAction == marble::EmitAST) {
        marble::ASTPrinter printer;
        for (auto stmt : mainMod->AST) {
            printer.Visit(stmt);
            llvm::outs() << '\n';
        }
        return 0; 
    }

    marble::SemanticAnalyzer sema(diag, srcMgr, libsPath, modManager);
    sema.Analyze(mainMod);
    if (diag.HasErrors()) {
        return 1;
    }
    diag.ResetErrors();
    
    marble::CodeGen codegen(mainMod, srcMgr);
    codegen.DeclareRuntimeFunctions();
    codegen.DeclareMod(mainMod);
    codegen.GenerateBodies(mainMod);

    llvm::Module *mod = codegen.GetLLVMModule();
    marble::InitializeLLVMTargets();
    
    std::string tripleStr = llvm::sys::getDefaultTargetTriple();
    llvm::Triple triple(tripleStr);
    llvm::StringRef stem = llvm::sys::path::stem(fileName);
    std::string outputName;
    if (!marble::OutputFilename.empty()) {
        outputName = marble::OutputFilename;
    }
    else {
        switch (marble::EmitAction) {
            case marble::EmitAST:
                outputName = "";
                break;
            case marble::EmitLLVM:
                outputName = (stem + ".ll").str();
                break;
            case marble::EmitObj:
                outputName = (stem + ".o").str();
                break;
            case marble::EmitBinary: 
                outputName = stem.str();
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
    
    std::string objFile = (marble::EmitAction == marble::EmitObj) ? outputName : (stem + ".o").str();
    if (!marble::EmitObjectFile(&*mod, objFile, tripleStr)) {
        return 1;
    }
    if (marble::EmitAction == marble::EmitBinary) {
        marble::LinkObjectFile(objFile, marble::GetOutputName(fileName, triple));
        llvm::sys::fs::remove(objFile); // NOLINT
    }
    llvm::outs().flush();   // explicitly flushing the buffer
    
    return 0;
}
