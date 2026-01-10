#pragma once
#include <onyx/AST/Node.h>

namespace onyx {
    class Expr : public Node {
    public:
        explicit Expr(NodeKind kind, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : Node(kind, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() > NkStartExprs && node->GetKind() < NkEndExprs;
        }
    };
}