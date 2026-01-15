#pragma once
#include <onyx/AST/Stmt.h>

namespace onyx {
    class ContinueStmt : public Stmt {
    public:
        explicit ContinueStmt(AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : Stmt(NkContinueStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const onyx::Node *node) {
            return node->GetKind() == NkContinueStmt;
        }
    };
}