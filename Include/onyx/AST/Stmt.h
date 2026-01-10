#pragma once
#include <onyx/AST/Node.h>

namespace onyx {
    class Stmt : public Node {
    public:
        explicit Stmt(NodeKind kind, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : Node(kind, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() > NkStartStmts && node->GetKind() < NkEndStmts;
        }
    };
}