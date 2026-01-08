#pragma once
#include <onyx/ast/Node.h>

namespace onyx {
    class Stmt : Node {
    public:
        explicit Stmt(NodeKind kind, llvm::SMLoc loc) : Node(kind, loc) {}

        static bool
        classof(const Node *node) {
            return node->GetKind() > NkStartStmts && node->GetKind() < NkEndStmts;
        }
    };
}