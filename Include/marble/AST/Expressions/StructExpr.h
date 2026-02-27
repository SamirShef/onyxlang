#pragma once
#include <marble/Basic/ASTType.h>
#include <marble/AST/Expr.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace marble {
    class StructExpr : public Expr {
        ASTType _type;
        std::vector<std::pair<std::string, Expr *>> _initializer;

    public:
        explicit StructExpr(ASTType type, std::vector<std::pair<std::string, Expr *>> initializer, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                          : _type(type), _initializer(initializer), Expr(NkStructExpr, startLoc, endLoc) {}
        
        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkStructExpr;
        }

        ASTType &
        GetType() {
            return _type;
        }

        std::vector<std::pair<std::string, Expr *>>
        GetInitializer() const {
            return _initializer;
        }
    };
}
