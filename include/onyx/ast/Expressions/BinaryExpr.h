#pragma once
#include <onyx/lexer/Token.h>
#include <onyx/ast/Expr.h>
#include <onyx/basic/ASTVal.h>

namespace onyx {
    class BinaryExpr : public Expr {
        Expr *_lhs;
        Expr *_rhs;
        Token _op;

    public:
        explicit BinaryExpr(Expr *lhs, Expr *rhs, Token op, llvm::SMLoc loc) : _lhs(lhs), _rhs(rhs), _op(op), Expr(NkBinaryExpr, loc) {}

        constexpr static bool
        classof(const onyx::Node *node) {
            return node->GetKind() == NkBinaryExpr;
        }

        Expr *
        GetLHS() const {
            return _lhs;
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