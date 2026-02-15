#pragma once
#include <marble/AST/Expr.h>
#include <marble/Basic/ASTType.h>

namespace marble {
    class NewExpr : public Expr {
        ASTType _type;

    public:
        explicit NewExpr(ASTType type, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _type(type), Expr(NkNewExpr, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkNewExpr;
        }

        ASTType
        GetType() const {
            return _type;
        }
    };
}
