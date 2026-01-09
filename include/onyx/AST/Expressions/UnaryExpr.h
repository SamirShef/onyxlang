#pragma once
#include <onyx/AST/Expr.h>
#include <onyx/Lexer/Token.h>

namespace onyx {
    class UnaryExpr : public Expr {
        Expr *_rhs;
        Token _op;

    public:
        explicit UnaryExpr(Expr *rhs, Token op, llvm::SMLoc loc) : _rhs(rhs), _op(op), Expr(NkBinaryExpr, loc) {}

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