#pragma once
#include <marble/AST/Stmt.h>
#include <marble/AST/Expr.h>
#include <marble/Basic/ASTType.h>
#include <llvm/ADT/StringRef.h>

namespace marble {
    class VarDeclStmt : public Stmt {
        std::string _name;
        bool _isConst;
        ASTType _type;
        Expr *_expr;

    public:
        explicit VarDeclStmt(std::string name, bool isConst, ASTType type, Expr *expr, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                           : _name(name), _type(type), _isConst(isConst), _expr(expr), Stmt(NkVarDeclStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkVarDeclStmt;
        }

        std::string
        GetName() const {
            return _name;
        }

        bool
        IsConst() const {
            return _isConst;
        }

        ASTType
        GetType() const {
            return _type;
        }

        void
        SetType(ASTType type) {
            _type = type;
        }
        
        Expr *
        GetExpr() const {
            return _expr;
        }
    };
}