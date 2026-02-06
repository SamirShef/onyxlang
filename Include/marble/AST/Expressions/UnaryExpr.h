#pragma once
#include <marble/AST/Expr.h>
#include <marble/Lexer/Token.h>

namespace marble {
    class UnaryExpr : public Expr {
        Expr *_rhs;
        Token _op;

    public:
        explicit UnaryExpr(Expr *rhs, Token op, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _rhs(rhs), _op(op), Expr(NkUnaryExpr, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkUnaryExpr;
        }

        Expr *
        GetRHS() const {
            return _rhs;
        }

        Token
        GetOp() const {
            return _op;
        }
    };
}