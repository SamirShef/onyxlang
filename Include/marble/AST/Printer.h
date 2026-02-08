#pragma once
#include <marble/AST/Visitor.h>

namespace marble {
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
        VisitTraitDeclStmt(TraitDeclStmt *tds);

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

        void
        VisitNilExpr(NilExpr *ne);

        void
        VisitDerefExpr(DerefExpr *de);

        void
        VisitRefExpr(RefExpr *re);
    };
}
