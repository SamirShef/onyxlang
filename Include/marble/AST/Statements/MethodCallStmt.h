#pragma once
#include <marble/AST/Expr.h>
#include <marble/AST/Stmt.h>
#include <marble/Basic/ASTType.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace marble {
    class MethodCallStmt : public Stmt {
        Expr *_object;
        std::string _name;
        std::vector<Expr *> _args;

    public:
        explicit MethodCallStmt(Expr *obj, std::string name, std::vector<Expr *> args, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                              : _object(obj), _name(name), _args(args), Stmt(NkMethodCallStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const marble::Node *node) {
            return node->GetKind() == NkMethodCallStmt;
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
    };
}
