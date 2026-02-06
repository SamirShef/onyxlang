#pragma once
#include <marble/AST/Stmt.h>

namespace marble {
    class ContinueStmt : public Stmt {
    public:
        explicit ContinueStmt(AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : Stmt(NkContinueStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const marble::Node *node) {
            return node->GetKind() == NkContinueStmt;
        }
    };
}