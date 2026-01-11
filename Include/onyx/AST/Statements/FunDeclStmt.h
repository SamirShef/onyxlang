#pragma once
#include <onyx/AST/Argument.h>
#include <onyx/AST/Stmt.h>
#include <onyx/Basic/ASTType.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace onyx {
    class FunDeclStmt : public Stmt {
        llvm::StringRef _name;
        ASTType _retType;
        std::vector<Argument> _args;
        std::vector<Stmt *> _block;

    public:
        explicit FunDeclStmt(llvm::StringRef name, ASTType retType, std::vector<Argument> args, std::vector<Stmt *> block, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                           : _name(name), _retType(retType), _args(args), _block(block), Stmt(NkFunDeclStmt, startLoc, endLoc) {}

        constexpr static bool
        classof(const onyx::Node *node) {
            return node->GetKind() == NkFunDeclStmt;
        }

        llvm::StringRef
        GetName() const {
            return _name;
        }

        ASTType
        GetRetType() const {
            return _retType;
        }

        std::vector<Argument>
        GetArgs() const {
            return _args;
        }

        std::vector<Stmt *>
        GetBody() const {
            return _block;
        }
    };
}