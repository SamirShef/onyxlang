#pragma once
#include <onyx/AST/AST.h>

namespace onyx {
    template<typename Derived, typename RetType = void>
    class ASTVisitor {
    public:
        RetType
        Visit(Node *node) {
            switch (node->GetKind()) {
                case NkVarDeclStmt:
                    return static_cast<Derived *>(this)->VisitVarDeclStmt(static_cast<VarDeclStmt *>(node));
                case NkVarAsgnStmt:
                    return static_cast<Derived *>(this)->VisitVarAsgnStmt(static_cast<VarAsgnStmt *>(node));
                case NkFunDeclStmt:
                    return static_cast<Derived *>(this)->VisitFunDeclStmt(static_cast<FunDeclStmt *>(node));
                case NkFunCallStmt:
                    return static_cast<Derived *>(this)->VisitFunCallStmt(static_cast<FunCallStmt *>(node));
                case NkRetStmt:
                    return static_cast<Derived *>(this)->VisitRetStmt(static_cast<RetStmt *>(node));
                case NkIfElseStmt:
                    return static_cast<Derived *>(this)->VisitIfElseStmt(static_cast<IfElseStmt *>(node));
                case NkForLoopStmt:
                    return static_cast<Derived *>(this)->VisitForLoopStmt(static_cast<ForLoopStmt *>(node));
                case NkBreakStmt:
                    return static_cast<Derived *>(this)->VisitBreakStmt(static_cast<BreakStmt *>(node));
                case NkContinueStmt:
                    return static_cast<Derived *>(this)->VisitContinueStmt(static_cast<ContinueStmt *>(node));
                case NkStructStmt:
                    return static_cast<Derived *>(this)->VisitStructStmt(static_cast<StructStmt *>(node));
                
                case NkBinaryExpr:
                    return static_cast<Derived *>(this)->VisitBinaryExpr(static_cast<BinaryExpr *>(node));
                case NkUnaryExpr:
                    return static_cast<Derived *>(this)->VisitUnaryExpr(static_cast<UnaryExpr *>(node));
                case NkVarExpr:
                    return static_cast<Derived *>(this)->VisitVarExpr(static_cast<VarExpr *>(node));
                case NkLiteralExpr:
                    return static_cast<Derived *>(this)->VisitLiteralExpr(static_cast<LiteralExpr *>(node));
                case NkFunCallExpr:
                    return static_cast<Derived *>(this)->VisitFunCallExpr(static_cast<FunCallExpr *>(node));
                case NkStructExpr:
                    return static_cast<Derived *>(this)->VisitStructExpr(static_cast<StructExpr *>(node));
                case NkFieldAccessExpr:
                    return static_cast<Derived *>(this)->VisitFieldAccessExpr(static_cast<FieldAccessExpr *>(node));
                case NkMethodCallExpr:
                    return static_cast<Derived *>(this)->VisitMethodCallExpr(static_cast<MethodCallExpr *>(node));
                default: {}
            }
        }
    };
}
