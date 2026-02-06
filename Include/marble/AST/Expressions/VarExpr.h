#pragma once
#include <marble/AST/Expr.h>
#include <llvm/ADT/StringRef.h>

namespace marble {
    class VarExpr : public Expr {
        std::string _name;

    public:
        explicit VarExpr(std::string name, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _name(name), Expr(NkVarExpr, startLoc, endLoc) {}
        
        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkVarExpr;
        }

        std::string
        GetName() const {
            return _name;
        }
    };
}