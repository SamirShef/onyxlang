#pragma once
#include <onyx/AST/Stmt.h>
#include <onyx/AST/Expr.h>
#include <vector>

namespace onyx {
    class ForLoopStmt : public Stmt {
        Stmt *_indexator;
        Expr *_cond;
        Stmt *_iteration;
        std::vector<Stmt *> _block;

    public:
        explicit ForLoopStmt(Stmt *indexator, Expr *cond, Stmt *iteration, std::vector<Stmt *> block, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                           : _indexator(indexator), _cond(cond), _iteration(iteration), _block(block), Stmt(NkForLoopStmt, startLoc, endLoc) {}

        constexpr static bool
        classof(const onyx::Node *node) {
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