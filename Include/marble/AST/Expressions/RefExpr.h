#pragma once
#include <marble/AST/Expr.h>

namespace marble {
    class RefExpr : public Expr {
        Expr *_expr;

    public:
        explicit RefExpr(Expr *expr, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _expr(expr), Expr(NkRefExpr, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkRefExpr;
        }

        Expr *
        GetExpr() const {
            return _expr;
        }
    };
}
