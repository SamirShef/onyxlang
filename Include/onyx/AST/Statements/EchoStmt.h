#pragma once
#include <onyx/AST/Stmt.h>
#include <onyx/AST/Expr.h>

namespace onyx {
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
