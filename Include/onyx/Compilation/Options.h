#pragma once
#include <llvm/Support/CommandLine.h>

namespace onyx {
    static llvm::cl::OptionCategory OnyxCat("Onyx Compiler Options");

    static llvm::cl::opt<std::string> InputFilename(
        llvm::cl::Positional, llvm::cl::desc("<input file>"), llvm::cl::Required, llvm::cl::cat(OnyxCat)
    );

    enum OptLevel {
        O0,
        O1,
        O2,
        O3
    };

    static llvm::cl::opt<OptLevel> OptimizationLevel(
        llvm::cl::desc("Optimization level:"),
        llvm::cl::values(
            clEnumValN(O0, "O0", "No optimization"),
            clEnumValN(O1, "O1", "Basic optimization"),
            clEnumValN(O2, "O2", "Default optimization"),
            clEnumValN(O3, "O3", "Aggressive optimization")),
        llvm::cl::init(O0), llvm::cl::cat(OnyxCat)
    );

    enum ActionKind {
        EmitAST,
        EmitLLVM,
        EmitObj,
        EmitBinary
    };

    static llvm::cl::opt<ActionKind> EmitAction(
        "emit", llvm::cl::desc("The kind of output to produce:"),
        llvm::cl::values(
            clEnumValN(EmitAST, "ast", "Print the AST to stdout"),
            clEnumValN(EmitLLVM, "llvm", "Emit LLVM IR (.ll)"),
            clEnumValN(EmitObj, "obj", "Emit object file (.o)"),
            clEnumValN(EmitBinary, "bin", "Emit executable (default)")),
        llvm::cl::init(EmitBinary), llvm::cl::cat(OnyxCat)
    );

    static llvm::cl::opt<std::string> OutputFilename(
        "o", llvm::cl::desc("Override output filename"), llvm::cl::value_desc("filename"), llvm::cl::cat(OnyxCat)
    );
}