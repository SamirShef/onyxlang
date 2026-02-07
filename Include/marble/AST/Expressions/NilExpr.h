#pragma once
#include <marble/AST/Expr.h>

namespace marble {
    class NilExpr : public Expr {
    public:
        explicit NilExpr(llvm::SMLoc startLoc, llvm::SMLoc endLoc) : Expr(NkNilExpr, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkNilExpr;
        }
    };
}
