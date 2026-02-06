#pragma once
#include <marble/AST/Expr.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace marble {
    class StructExpr : public Expr {
        std::string _name;
        std::vector<std::pair<std::string, Expr *>> _initializer;

    public:
        explicit StructExpr(std::string name, std::vector<std::pair<std::string, Expr *>> initializer, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                          : _name(name), _initializer(initializer), Expr(NkStructExpr, startLoc, endLoc) {}
        
        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkStructExpr;
        }

        std::string
        GetName() const {
            return _name;
        }

        std::vector<std::pair<std::string, Expr *>>
        GetInitializer() const {
            return _initializer;
        }
    };
}