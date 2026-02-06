#pragma once
#include <marble/AST/Expr.h>
#include <llvm/ADT/StringRef.h>

namespace marble {
    class FieldAccessExpr : public Expr {
        Expr *_object;
        llvm::StringRef _name;

    public:
        explicit FieldAccessExpr(Expr *obj, llvm::StringRef name, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                               : _object(obj), _name(name), Expr(NkFieldAccessExpr, startLoc, endLoc) {}
        
        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkFieldAccessExpr;
        }

        Expr *
        GetObject() const {
            return _object;
        }

        llvm::StringRef
        GetName() const {
            return _name;
        }
    };
}
