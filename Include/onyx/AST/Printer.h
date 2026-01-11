#pragma once
#include <onyx/AST/Visitor.h>

namespace onyx {
    class ASTPrinter : public ASTVisitor<ASTPrinter> {
        int _spaces = 0;
    public:
        void
        visitVarDeclStmt(VarDeclStmt *vds);

        void
        visitFunDeclStmt(FunDeclStmt *fds);

        void
        visitRetStmt(RetStmt *rs);

        void
        visitBinaryExpr(BinaryExpr *be);

        void
        visitUnaryExpr(UnaryExpr *ue);

        void
        visitVarExpr(VarExpr *ve);

        void
        visitLiteralExpr(LiteralExpr *le);

        void
        visitFunCallExpr(FunCallExpr *fce);
    };
}