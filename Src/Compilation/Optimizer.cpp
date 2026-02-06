#include <marble/Compilation/Optimizer.h>

namespace marble {
    void
    Optimizer::Optimize(llvm::Module &mod, Level level) {
        if (level == Level::O0) {
            return;
        }

        llvm::LoopAnalysisManager LAM;
        llvm::FunctionAnalysisManager FAM;
        llvm::CGSCCAnalysisManager CGAM;
        llvm::ModuleAnalysisManager MAM;

        llvm::PassBuilder PB;

        PB.registerModuleAnalyses(MAM);
        PB.registerCGSCCAnalyses(CGAM);
        PB.registerFunctionAnalyses(FAM);
        PB.registerLoopAnalyses(LAM);
        PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

        llvm::OptimizationLevel LLVMLvl;
        switch (level) {
            case Level::O1:
                LLVMLvl = llvm::OptimizationLevel::O1;
                break;
            case Level::O2:
                LLVMLvl = llvm::OptimizationLevel::O2;
                break;
            case Level::O3:
                LLVMLvl = llvm::OptimizationLevel::O3;
                break;
            default:
                LLVMLvl = llvm::OptimizationLevel::O0;
                break;
        }

        llvm::ModulePassManager MPM;

        MPM = PB.buildPerModuleDefaultPipeline(LLVMLvl);

        MPM.run(mod, MAM);
    }
}
