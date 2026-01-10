#pragma once
#include <onyx/AST/Expr.h>
#include <onyx/Basic/ASTVal.h>

namespace onyx {
    class LiteralExpr : public Expr {
        ASTVal _val;

    public:
        explicit LiteralExpr(ASTVal val, llvm::SMLoc loc) : _val(val), Expr(NkLiteralExpr, loc) {}

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