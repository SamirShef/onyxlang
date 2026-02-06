#pragma once
#include <marble/AST/Stmt.h>

namespace marble {
    class BreakStmt : public Stmt {
    public:
        explicit BreakStmt(AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : Stmt(NkBreakStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkBreakStmt;
        }
    };
}
