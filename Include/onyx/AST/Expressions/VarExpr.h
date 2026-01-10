#pragma once
#include <onyx/AST/Expr.h>
#include <llvm/ADT/StringRef.h>

namespace onyx {
    class VarExpr : public Expr {
        llvm::StringRef _name;

    public:
        explicit VarExpr(llvm::StringRef name, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _name(name), Expr(NkVarExpr, startLoc, endLoc) {}
        
        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkVarExpr;
        }

        llvm::StringRef
        GetName() const {
            return _name;
        }
    };
}