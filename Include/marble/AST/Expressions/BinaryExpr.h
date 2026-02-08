#pragma once
#include <marble/AST/Expr.h>
#include <marble/Basic/ASTType.h>
#include <marble/Lexer/Token.h>

namespace marble {
    class BinaryExpr : public Expr {
        Expr *_lhs;
        Expr *_rhs;
        Token _op;

        ASTType _lhsType;
        ASTType _rhsType;

    public:
        explicit BinaryExpr(Expr *lhs, Expr *rhs, Token op, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _lhs(lhs), _rhs(rhs), _op(op), _lhsType(ASTType::GetNothType()),
                                                                                                        _rhsType(ASTType::GetNothType()), Expr(NkBinaryExpr, startLoc, endLoc) {}

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

        void
        SetLHSType(ASTType t) {
            _lhsType = t;
        }

        void
        SetRHSType(ASTType t) {
            _rhsType = t;
        }

        ASTType
        GetLHSType() const {
            return _lhsType;
        }

        ASTType
        GetRHSType() const {
            return _rhsType;
        }
    };
}
