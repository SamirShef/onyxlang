#pragma once
#include <onyx/AST/Expr.h>
#include <onyx/AST/Stmt.h>
#include <onyx/Basic/ASTType.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace onyx {
    class FunCallStmt : public Stmt {
        llvm::StringRef _name;
        std::vector<Expr *> _args;

    public:
        explicit FunCallStmt(llvm::StringRef name, std::vector<Expr *> args, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                           : _name(name), _args(args), Stmt(NkFunCallStmt, startLoc, endLoc) {}

        constexpr static bool
        classof(const onyx::Node *node) {
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