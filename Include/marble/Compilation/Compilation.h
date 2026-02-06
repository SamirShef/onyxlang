#pragma once
#include <llvm/IR/Module.h>
#include <string>

namespace marble {
    void
    InitializeLLVMTargets();

    bool
    EmitObjectFile(llvm::Module *mod, const std::string &fileName, std::string targetTripleStr = "");

    void
    LinkObjectFile(const std::string &objFile, const std::string &exeFile);

    std::string
    GetOutputName(const std::string &inputFile, const llvm::Triple &triple);
}