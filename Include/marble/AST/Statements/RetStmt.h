#pragma once
#include <marble/AST/Stmt.h>
#include <marble/AST/Expr.h>

namespace marble {
    class RetStmt : public Stmt {
        Expr *_expr;

    public:
        explicit RetStmt(Expr *expr, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _expr(expr), Stmt(NkRetStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkRetStmt;
        }
        
        Expr *
        GetExpr() const {
            return _expr;
        }
    };
}