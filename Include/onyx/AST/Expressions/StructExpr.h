#pragma once
#include <onyx/AST/Expr.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace onyx {
    class StructExpr : public Expr {
        llvm::StringRef _name;
        std::vector<std::pair<llvm::StringRef, Expr *>> _initializer;

    public:
        explicit StructExpr(llvm::StringRef name, std::vector<std::pair<llvm::StringRef, Expr *>> initializer, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                          : _name(name), _initializer(initializer), Expr(NkStructExpr, startLoc, endLoc) {}
        
        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkStructExpr;
        }

        llvm::StringRef
        GetName() const {
            return _name;
        }

        std::vector<std::pair<llvm::StringRef, Expr *>>
        GetInitializer() const {
            return _initializer;
        }
    };
}