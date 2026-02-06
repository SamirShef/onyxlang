#pragma once
#include <marble/AST/Expr.h>
#include <marble/Basic/ASTType.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace marble {
    class FunCallExpr : public Expr {
        std::string _name;
        std::vector<Expr *> _args;

    public:
        explicit FunCallExpr(std::string name, std::vector<Expr *> args, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                           : _name(name), _args(args), Expr(NkFunCallExpr, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkFunCallExpr;
        }

        std::string
        GetName() const {
            return _name;
        }

        std::vector<Expr *>
        GetArgs() const {
            return _args;
        }
    };
}