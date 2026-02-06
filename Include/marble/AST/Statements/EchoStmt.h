#pragma once
#include <marble/AST/Stmt.h>
#include <marble/AST/Expr.h>

namespace marble {
    class EchoStmt : public Stmt {
        Expr *_rhs;

    public:
        explicit EchoStmt(Expr *rhs, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _rhs(rhs), Stmt(NkEchoStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkEchoStmt;
        }

        Expr *
        GetRHS() const {
            return _rhs;
        }
    };
}
