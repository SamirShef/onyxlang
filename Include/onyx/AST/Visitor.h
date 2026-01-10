#pragma once
#include <onyx/AST/AST.h>

namespace onyx {
    template<typename Derived>
    class ASTVisitor {
    public:
        void
        visit(Node *node) {
            switch (node->GetKind()) {
                case NkVarDeclStmt:
                    static_cast<Derived*>(this)->visitVarDeclStmt(static_cast<VarDeclStmt*>(node));
                    break;
                case NkBinaryExpr:
                    static_cast<Derived*>(this)->visitBinaryExpr(static_cast<BinaryExpr*>(node));
                    break;
                case NkUnaryExpr:
                    static_cast<Derived*>(this)->visitUnaryExpr(static_cast<UnaryExpr*>(node));
                    break;
                case NkVarExpr:
                    static_cast<Derived*>(this)->visitVarExpr(static_cast<VarExpr*>(node));
                    break;
                case NkLiteralExpr:
                    static_cast<Derived*>(this)->visitLiteralExpr(static_cast<LiteralExpr*>(node));
                    break;
                default: {}
            }
        }
    };
}