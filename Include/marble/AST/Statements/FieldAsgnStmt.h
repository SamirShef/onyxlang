#pragma once
#include <marble/AST/Stmt.h>
#include <marble/AST/Expr.h>
#include <marble/Basic/ASTType.h>
#include <llvm/ADT/StringRef.h>

namespace marble {
    class FieldAsgnStmt : public Stmt {
        Expr *_object;
        std::string _name;
        Expr *_expr;

    public:
        explicit FieldAsgnStmt(Expr *obj, std::string name, Expr *expr, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                             : _object(obj), _name(name), _expr(expr), Stmt(NkFieldAsgnStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkFieldAsgnStmt;
        }

        Expr *
        GetObject() const {
            return _object;
        }

        std::string
        GetName() const {
            return _name;
        }
        
        Expr *
        GetExpr() const {
            return _expr;
        }
    };
}
