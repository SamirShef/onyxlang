#pragma once
#include <onyx/AST/Visitor.h>

namespace onyx {
    class ASTPrinter : public ASTVisitor<ASTPrinter> {
        int _spaces = 0;
    public:
        void
        VisitVarDeclStmt(VarDeclStmt *vds);

        void
        VisitVarAsgnStmt(VarAsgnStmt *vas);

        void
        VisitFunDeclStmt(FunDeclStmt *fds);

        void
        VisitFunCallStmt(FunCallStmt *fcs);

        void
        VisitRetStmt(RetStmt *rs);

        void
        VisitIfElseStmt(IfElseStmt *ies);
        
        void
        VisitForLoopStmt(ForLoopStmt *fls);

        void
        VisitBreakStmt(BreakStmt *bs);

        void
        VisitContinueStmt(ContinueStmt *cs);

        void
        VisitStructStmt(StructStmt *ss);

        void
        VisitImplStmt(ImplStmt *is);

        void
        VisitMethodCallStmt(MethodCallStmt *mcs);

        void
        VisitEchoStmt(EchoStmt *es);

        void
        VisitFieldAsgnStmt(FieldAsgnStmt *fas);

        void
        VisitBinaryExpr(BinaryExpr *be);

        void
        VisitUnaryExpr(UnaryExpr *ue);

        void
        VisitVarExpr(VarExpr *ve);

        void
        VisitLiteralExpr(LiteralExpr *le);

        void
        VisitFunCallExpr(FunCallExpr *fce);

        void
        VisitStructExpr(StructExpr *se);

        void
        VisitFieldAccessExpr(FieldAccessExpr *fae);

        void
        VisitMethodCallExpr(MethodCallExpr *mce);
    };
}
