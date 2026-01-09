#pragma once
#include <llvm/Support/SMLoc.h>

namespace onyx {
    enum NodeKind {
        NkStartStmts,
        NkVarDeclStmt,
        NkEndStmts,

        NkStartExprs,
        NkBinaryExpr,
        NkUnaryExpr,
        NkVarExpr,
        NkLiteralExpr,
        NkEndExprs,
    };

    class Node {
        const NodeKind _kind;
        llvm::SMLoc _loc;

    public:
        explicit Node(NodeKind kind, llvm::SMLoc loc) : _kind(kind), _loc(loc) {}
        virtual ~Node() = default;

        NodeKind
        GetKind() const {
            return _kind;
        }

        const llvm::SMLoc
        GetLoc() const {
            return _loc;
        }
    };
}