#pragma once
#include <marble/AST/Expr.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace marble {
    class MethodCallExpr : public Expr {
        Expr *_object;
        llvm::StringRef _name;
        std::vector<Expr *> _args;

    public:
        explicit MethodCallExpr(Expr *obj, llvm::StringRef name, std::vector<Expr *> args, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                              : _object(obj), _name(name), _args(args), Expr(NkMethodCallExpr, startLoc, endLoc) {}
        
        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkMethodCallExpr;
        }

        Expr *
        GetObject() const {
            return _object;
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
