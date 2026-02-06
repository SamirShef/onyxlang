#pragma once
#include <marble/AST/Stmt.h>
#include <marble/AST/Expr.h>
#include <vector>

namespace marble {
    class IfElseStmt : public Stmt {
        Expr *_cond;
        std::vector<Stmt *> _thenBranch;
        std::vector<Stmt *> _elseBranch;

    public:
        explicit IfElseStmt(Expr *cond, std::vector<Stmt *> thenBranch, std::vector<Stmt *> elseBranch, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                          : _cond(cond), _thenBranch(thenBranch), _elseBranch(elseBranch), Stmt(NkIfElseStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkIfElseStmt;
        }
        
        Expr *
        GetCondition() const {
            return _cond;
        }

        std::vector<Stmt *>
        GetThenBody() const {
            return _thenBranch;
        }

        std::vector<Stmt *>
        GetElseBody() const {
            return _elseBranch;
        }
    };
}