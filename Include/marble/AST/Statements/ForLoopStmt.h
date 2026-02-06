#pragma once
#include <marble/AST/Stmt.h>
#include <marble/AST/Expr.h>
#include <vector>

namespace marble {
    class ForLoopStmt : public Stmt {
        Stmt *_indexator;
        Expr *_cond;
        Stmt *_iteration;
        std::vector<Stmt *> _block;

    public:
        explicit ForLoopStmt(Stmt *indexator, Expr *cond, Stmt *iteration, std::vector<Stmt *> block, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                           : _indexator(indexator), _cond(cond), _iteration(iteration), _block(block), Stmt(NkForLoopStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const marble::Node *node) {
            return node->GetKind() == NkForLoopStmt;
        }
        
        Stmt *
        GetIndexator() const {
            return _indexator;
        }
        
        Expr *
        GetCondition() const {
            return _cond;
        }

        Stmt *
        GetIteration() const {
            return _iteration;
        }

        std::vector<Stmt *>
        GetBody() const {
            return _block;
        }
    };
}