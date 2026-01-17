#pragma once
#include <onyx/AST/Stmt.h>
#include <onyx/AST/Expr.h>
#include <onyx/Basic/ASTType.h>
#include <llvm/ADT/StringRef.h>

namespace onyx {
    class FieldAsgnStmt : public Stmt {
        Expr *_object;
        llvm::StringRef _name;
        Expr *_expr;

    public:
        explicit FieldAsgnStmt(Expr *obj, llvm::StringRef name, Expr *expr, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                           : _object(obj), _name(name), _expr(expr), Stmt(NkFieldAsgnStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const onyx::Node *node) {
            return node->GetKind() == NkFieldAsgnStmt;
        }

        Expr *
        GetObject() const {
            return _object;
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
