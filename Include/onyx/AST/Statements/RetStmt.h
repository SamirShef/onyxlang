#pragma once
#include <onyx/AST/Stmt.h>
#include <onyx/AST/Expr.h>

namespace onyx {
    class RetStmt : public Stmt {
        Expr *_expr;

    public:
        explicit RetStmt(Expr *expr, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _expr(expr), Stmt(NkRetStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const onyx::Node *node) {
            return node->GetKind() == NkRetStmt;
        }
        
        Expr *
        GetExpr() const {
            return _expr;
        }
    };
}