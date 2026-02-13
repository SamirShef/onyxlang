#pragma once
#include <marble/AST/Node.h>

namespace marble {
    class Stmt : public Node {
        AccessModifier _access;
    public:
        explicit Stmt(NodeKind kind, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc) : _access(access), Node(kind, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() > NkStartStmts && node->GetKind() < NkEndStmts;
        }

        AccessModifier
        GetAccess() const {
            return _access;
        }
    };
}
