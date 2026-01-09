#pragma once
#include <onyx/ast/Node.h>
#include <onyx/ast/Stmt.h>
#include <onyx/ast/Expr.h>
#include <onyx/basic/ASTType.h>
#include <llvm/ADT/StringRef.h>

namespace onyx {
    class VarDeclStmt : public Stmt {
        llvm::StringRef _name;
        bool _isConst;
        ASTType _type;
        Expr *_expr;

    public:
        explicit VarDeclStmt(llvm::StringRef name, bool isConst, ASTType type, Expr *expr, llvm::SMLoc loc) : _name(name), _type(type), _isConst(isConst),
                                                                                                              _expr(expr), Stmt(NkVarDeclStmt, loc) {}

        constexpr static bool
        classof(const onyx::Node *node) {
            return node->GetKind() == NkVarDeclStmt;
        }

        llvm::StringRef
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
        
        Expr *
        GetExpr() const {
            return _expr;
        }
    };
}