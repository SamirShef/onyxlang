#pragma once
#include <marble/AST/Expr.h>
#include <marble/Basic/ASTType.h>
#include <string>

namespace marble {
    class FieldAccessExpr : public Expr {
        Expr *_object;
        std::string _name;
        ASTType _objType;

    public:
        explicit FieldAccessExpr(Expr *obj, std::string name, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                               : _object(obj), _name(name), Expr(NkFieldAccessExpr, startLoc, endLoc) {}
        
        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkFieldAccessExpr;
        }

        Expr *
        GetObject() const {
            return _object;
        }

        std::string
        GetName() const {
            return _name;
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
