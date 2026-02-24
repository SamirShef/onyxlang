#pragma once
#include <marble/AST/Expr.h>
#include <marble/Basic/ASTType.h>
#include <string>
#include <vector>

namespace marble {
    class MethodCallExpr : public Expr {
        Expr *_object;
        std::string _name;
        std::vector<Expr *> _args;
        ASTType _objType;

    public:
        explicit MethodCallExpr(Expr *obj, std::string name, std::vector<Expr *> args, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                              : _object(obj), _name(name), _args(args), Expr(NkMethodCallExpr, startLoc, endLoc) {}
        
        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkMethodCallExpr;
        }

        Expr *
        GetObject() const {
            return _object;
        }

        std::string
        GetName() const {
            return _name;
        }

        std::vector<Expr *>
        GetArgs() const {
            return _args;
        }

        ASTType
        GetObjType() const {
            return _objType;
        }

        void
        SetObjType(ASTType t) {
            _objType = t;
        }
    };
}
