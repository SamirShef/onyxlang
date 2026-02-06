#pragma once
#include <marble/AST/Expr.h>
#include <marble/AST/Stmt.h>
#include <marble/Basic/ASTType.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace marble {
    class FunCallStmt : public Stmt {
        llvm::StringRef _name;
        std::vector<Expr *> _args;

    public:
        explicit FunCallStmt(llvm::StringRef name, std::vector<Expr *> args, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                           : _name(name), _args(args), Stmt(NkFunCallStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const marble::Node *node) {
            return node->GetKind() == NkFunCallStmt;
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