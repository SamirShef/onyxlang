#pragma once
#include <onyx/AST/AST.h>

namespace onyx {
    template<typename Derived, typename RetType = void>
    class ASTVisitor {
    public:
        RetType
        Visit(Node *node) {
            #define NODE(fun, obj) static_cast<Derived *>(this)->fun(static_cast<obj *>(node))
            switch (node->GetKind()) {
                case NkVarDeclStmt:
                    return NODE(VisitVarDeclStmt, VarDeclStmt);
                case NkVarAsgnStmt:
                    return NODE(VisitVarAsgnStmt, VarAsgnStmt);
                case NkFieldAsgnStmt:
                    return NODE(VisitFieldAsgnStmt, FieldAsgnStmt);
                case NkFunDeclStmt:
                    return NODE(VisitFunDeclStmt, FunDeclStmt);
                case NkFunCallStmt:
                    return NODE(VisitFunCallStmt, FunCallStmt);
                case NkRetStmt:
                    return NODE(VisitRetStmt, RetStmt);
                case NkIfElseStmt:
                    return NODE(VisitIfElseStmt, IfElseStmt);
                case NkForLoopStmt:
                    return NODE(VisitForLoopStmt, ForLoopStmt);
                case NkBreakStmt:
                    return NODE(VisitBreakStmt, BreakStmt);
                case NkContinueStmt:
                    return NODE(VisitContinueStmt, ContinueStmt);
                case NkStructStmt:
                    return NODE(VisitStructStmt, StructStmt);
                
                case NkBinaryExpr:
                    return NODE(VisitBinaryExpr, BinaryExpr);
                case NkUnaryExpr:
                    return NODE(VisitUnaryExpr, UnaryExpr);
                case NkVarExpr:
                    return NODE(VisitVarExpr, VarExpr);
                case NkLiteralExpr:
                    return NODE(VisitLiteralExpr, LiteralExpr);
                case NkFunCallExpr:
                    return NODE(VisitFunCallExpr, FunCallExpr);
                case NkStructExpr:
                    return NODE(VisitStructExpr, StructExpr);
                case NkFieldAccessExpr:
                    return NODE(VisitFieldAccessExpr, FieldAccessExpr);
                case NkMethodCallExpr:
                    return NODE(VisitMethodCallExpr, MethodCallExpr);
                default: {}
            }
            #undef NODE
        }
    };
}
