#pragma once
#include <onyx/ast/Node.h>

namespace onyx {
    class Expr : public Node {
    public:
        explicit Expr(NodeKind kind, llvm::SMLoc loc) : Node(kind, loc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() > NkStartExprs && node->GetKind() < NkEndExprs;
        }
    };
}