#pragma once
#include <marble/AST/Expr.h>

namespace marble {
    class DerefExpr : public Expr {
        Expr *_expr;

    public:
        explicit DerefExpr(Expr *expr, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _expr(expr), Expr(NkDerefExpr, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkDerefExpr;
        }

        Expr *
        GetExpr() const {
            return _expr;
        }
    };
}
