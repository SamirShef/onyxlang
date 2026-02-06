#pragma once
#include <marble/AST/Expr.h>
#include <marble/Basic/ASTVal.h>

namespace marble {
    class LiteralExpr : public Expr {
        ASTVal _val;

    public:
        explicit LiteralExpr(ASTVal val, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _val(val), Expr(NkLiteralExpr, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkLiteralExpr;
        }
        
        ASTVal
        GetVal() const {
            return _val;
        }
    };
}