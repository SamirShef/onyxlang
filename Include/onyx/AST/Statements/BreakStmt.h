#pragma once
#include <onyx/AST/Stmt.h>

namespace onyx {
    class BreakStmt : public Stmt {
    public:
        explicit BreakStmt(AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : Stmt(NkBreakStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const onyx::Node *node) {
            return node->GetKind() == NkBreakStmt;
        }
    };
}