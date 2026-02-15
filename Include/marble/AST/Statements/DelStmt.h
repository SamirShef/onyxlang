#pragma once
#include <marble/AST/Stmt.h>
#include <marble/AST/Expr.h>
#include <marble/Basic/ASTType.h>

namespace marble {
    class DelStmt : public Stmt {
        Expr *_expr;

    public:
        explicit DelStmt(Expr *expr, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _expr(expr), Stmt(NkDelStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkDelStmt;
        }

        Expr *
        GetExpr() const {
            return _expr;
        }
    };
}
