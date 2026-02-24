#pragma once
#include <marble/Basic/ASTType.h>
#include <marble/AST/Stmt.h>
#include <marble/AST/Expr.h>

namespace marble {
    class EchoStmt : public Stmt {
        Expr *_expr;
        ASTType _type;

    public:
        explicit EchoStmt(Expr *expr, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _expr(expr), Stmt(NkEchoStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkEchoStmt;
        }

        Expr *
        GetExpr() const {
            return _expr;
        }

        ASTType
        GetExprType() const {
            return _type;
        }

        void
        SetExprType(ASTType type) {
            _type = type;
        }
    };
}
