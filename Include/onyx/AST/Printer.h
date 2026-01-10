#pragma once
#include <onyx/AST/Visitor.h>

namespace onyx {
    class ASTPrinter : public ASTVisitor<ASTPrinter> {
        int _spaces = 0;
    public:
        void
        visitVarDeclStmt(VarDeclStmt *vds);
        void
        visitBinaryExpr(BinaryExpr *be);
        void
        visitUnaryExpr(UnaryExpr *ue);
        void
        visitVarExpr(VarExpr *ve);
        void
        visitLiteralExpr(LiteralExpr *le);
    };
}