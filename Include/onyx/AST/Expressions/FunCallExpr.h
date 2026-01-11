#pragma once
#include <onyx/AST/Expr.h>
#include <onyx/Basic/ASTType.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace onyx {
    class FunCallExpr : public Expr {
        llvm::StringRef _name;
        std::vector<Expr *> _args;

    public:
        explicit FunCallExpr(llvm::StringRef name, std::vector<Expr *> args, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                           : _name(name), _args(args), Expr(NkFunCallExpr, startLoc, endLoc) {}

        constexpr static bool
        classof(const onyx::Node *node) {
            return node->GetKind() == NkFunCallExpr;
        }

        llvm::StringRef
        GetName() const {
            return _name;
        }

        std::vector<Expr *>
        GetArgs() const {
            return _args;
        }
    };
}