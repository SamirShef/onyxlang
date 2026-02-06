#pragma once
#include <marble/AST/Stmt.h>
#include <marble/AST/Expr.h>
#include <marble/Basic/ASTType.h>
#include <llvm/ADT/StringRef.h>

namespace marble {
    class VarAsgnStmt : public Stmt {
        std::string _name;
        Expr *_expr;

    public:
        explicit VarAsgnStmt(std::string name, Expr *expr, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                           : _name(name), _expr(expr), Stmt(NkVarAsgnStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const marble::Node *node) {
            return node->GetKind() == NkVarAsgnStmt;
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