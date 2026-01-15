#pragma once
#include <onyx/AST/Stmt.h>
#include <onyx/AST/Expr.h>
#include <onyx/Basic/ASTType.h>
#include <llvm/ADT/StringRef.h>

namespace onyx {
    class VarAsgnStmt : public Stmt {
        llvm::StringRef _name;
        Expr *_expr;

    public:
        explicit VarAsgnStmt(llvm::StringRef name, Expr *expr, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                           : _name(name), _expr(expr), Stmt(NkVarAsgnStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const onyx::Node *node) {
            return node->GetKind() == NkVarAsgnStmt;
        }

        llvm::StringRef
        GetName() const {
            return _name;
        }
        
        Expr *
        GetExpr() const {
            return _expr;
        }
    };
}