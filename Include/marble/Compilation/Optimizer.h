#pragma once
#include <llvm/IR/Module.h>
#include <llvm/Passes/PassBuilder.h>

namespace marble {
    class Optimizer {
    public:
        enum Level {
            O0,
            O1,
            O2,
            O3
        };

        static void
        Optimize(llvm::Module &M, Level L);
    };
}