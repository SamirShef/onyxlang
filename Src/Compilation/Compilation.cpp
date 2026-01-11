#include <onyx/Compilation/Compilation.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/CodeGen.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

namespace onyx {
    void
    InitializeLLVMTargets() {
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmParsers();
        llvm::InitializeAllAsmPrinters();
    }

    bool
    EmitObjectFile(llvm::Module *mod, const std::string &fileName, std::string targetTripleStr) {
        if (targetTripleStr.empty()) {
            targetTripleStr = llvm::sys::getDefaultTargetTriple();
        }

        llvm::Triple triple(targetTripleStr);

        mod->setTargetTriple(triple);

        std::string error;
        const llvm::Target *target = llvm::TargetRegistry::lookupTarget(triple.getTriple(), error);

        if (!target) {
            llvm::errs() << llvm::errs().RED << "Error looking up target: " << llvm::errs().RESET << error << '\n';
            return false;
        }

        std::string cpu = "generic";
        std::string features = ""; 

        llvm::TargetOptions opt;
        std::optional<llvm::Reloc::Model> rm = llvm::Reloc::Model::PIC_;
        
        auto *targetMachine = target->createTargetMachine(triple.getTriple(), cpu, features, opt, rm);

        mod->setDataLayout(targetMachine->createDataLayout());

        std::error_code ec;
        llvm::raw_fd_ostream dest(fileName, ec, llvm::sys::fs::OF_None);

        if (ec) {
            llvm::errs() << llvm::errs().RED << "Could not open file: " << ec.message() << '\n' << llvm::errs().RESET;
            return false;
        }

        llvm::legacy::PassManager pass;
        auto fileType = llvm::CodeGenFileType::ObjectFile;

        if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, fileType)) {
            llvm::errs() << llvm::errs().RED << "TargetMachine can't emit a file of this type\n" << llvm::errs().RESET;
            return false;
        }

        pass.run(*mod);
        dest.flush();

        return true;
    }

    void
    LinkObjectFile(const std::string &objFile, const std::string &exeFile) {
        std::string cmd = "clang " + objFile + " -o " + exeFile;
        system(cmd.c_str());
    }

    std::string
    GetOutputName(const std::string &inputFile, const llvm::Triple &triple) {
        llvm::StringRef stem = llvm::sys::path::stem(inputFile);
        
        std::string outputExe = stem.str();

        if (triple.isOSWindows()) {
            outputExe += ".exe";
        }
        
        return outputExe;
    }
}