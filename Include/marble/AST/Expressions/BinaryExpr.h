#pragma once
#include <marble/AST/Expr.h>
#include <marble/Lexer/Token.h>

namespace marble {
    class BinaryExpr : public Expr {
        Expr *_lhs;
        Expr *_rhs;
        Token _op;

    public:
        explicit BinaryExpr(Expr *lhs, Expr *rhs, Token op, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _lhs(lhs), _rhs(rhs), _op(op),
                                                                                                        Expr(NkBinaryExpr, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
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