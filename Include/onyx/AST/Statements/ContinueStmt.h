#pragma once
#include <onyx/AST/Stmt.h>

namespace onyx {
    class ContinueStmt : public Stmt {
    public:
        explicit ContinueStmt(llvm::SMLoc startLoc, llvm::SMLoc endLoc) : Stmt(NkContinueStmt, startLoc, endLoc) {}

        constexpr static bool
        classof(const onyx::Node *node) {
            return node->GetKind() == NkContinueStmt;
        }
    };
}