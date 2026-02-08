#pragma once
#include <marble/AST/Expr.h>
#include <marble/Basic/ASTType.h>

namespace marble {
    class DerefExpr : public Expr {
        Expr *_expr;
        ASTType _exprType;

    public:
        explicit DerefExpr(Expr *expr, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _expr(expr), _exprType(ASTType::GetNothType()), Expr(NkDerefExpr, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkDerefExpr;
        }

        Expr *
        GetExpr() const {
            return _expr;
        }

        void
        SetExprType(ASTType t) {
            _exprType = t;
        }

        ASTType
        GetExprType() const {
            return _exprType;
        }
    };
}
