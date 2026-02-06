#pragma once
#include <marble/AST/Node.h>

namespace marble {
    class Expr : public Node {
    public:
        explicit Expr(NodeKind kind, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : Node(kind, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() > NkStartExprs && node->GetKind() < NkEndExprs;
        }
    };
}