#pragma once
#include <onyx/AST/AST.h>

namespace onyx {
    template<typename Derived, typename RetType = void>
    class ASTVisitor {
    public:
        RetType
        visit(Node *node) {
            switch (node->GetKind()) {
                case NkVarDeclStmt:
                    return static_cast<Derived*>(this)->visitVarDeclStmt(static_cast<VarDeclStmt*>(node));
                case NkBinaryExpr:
                    return static_cast<Derived*>(this)->visitBinaryExpr(static_cast<BinaryExpr*>(node));
                case NkUnaryExpr:
                    return static_cast<Derived*>(this)->visitUnaryExpr(static_cast<UnaryExpr*>(node));
                case NkVarExpr:
                    return static_cast<Derived*>(this)->visitVarExpr(static_cast<VarExpr*>(node));
                case NkLiteralExpr:
                    return static_cast<Derived*>(this)->visitLiteralExpr(static_cast<LiteralExpr*>(node));
                default: {}
            }
        }
    };
}