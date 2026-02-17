#pragma once
#include <llvm/Support/SMLoc.h>

namespace marble {
    enum NodeKind {
        NkStartStmts,
        NkVarDeclStmt,
        NkVarAsgnStmt,
        NkFieldAsgnStmt,
        NkFunDeclStmt,
        NkFunCallStmt,
        NkRetStmt,
        NkIfElseStmt,
        NkForLoopStmt,
        NkBreakStmt,
        NkContinueStmt,
        NkStructStmt,
        NkImplStmt,
        NkMethodCallStmt,
        NkTraitDeclStmt,
        NkEchoStmt,
        NkDelStmt,
        NkImportStmt,
        NkModuleDeclStmt,
        NkEndStmts,

        NkStartExprs,
        NkBinaryExpr,
        NkUnaryExpr,
        NkVarExpr,
        NkLiteralExpr,
        NkFunCallExpr,
        NkStructExpr,
        NkFieldAccessExpr,
        NkMethodCallExpr,
        NkNilExpr,
        NkNewExpr,
        NkDerefExpr,
        NkRefExpr,
        NkEndExprs,
    };

    enum AccessModifier {
        AccessPriv,
        AccessPub,
    };

    class Node {
        const NodeKind _kind;
        llvm::SMLoc _startLoc;
        llvm::SMLoc _endLoc;

    public:
        explicit Node(NodeKind kind, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _kind(kind), _startLoc(startLoc), _endLoc(endLoc) {}
        virtual ~Node() = default;

        NodeKind
        GetKind() const {
            return _kind;
        }

        const llvm::SMLoc
        GetStartLoc() const {
            return _startLoc;
        }

        const llvm::SMLoc
        GetEndLoc() const {
            return _endLoc;
        }

        void
        SetStartLoc(llvm::SMLoc startLoc) {
            _startLoc = startLoc;
        }

        void
        SetEndLoc(llvm::SMLoc endLoc) {
            _endLoc = endLoc;
        }
    };
}
