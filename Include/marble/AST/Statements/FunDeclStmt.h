#pragma once
#include <marble/AST/Argument.h>
#include <marble/AST/Stmt.h>
#include <marble/Basic/ASTType.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace marble {
    class FunDeclStmt : public Stmt {
        std::string _name;
        ASTType _retType;
        std::vector<Argument> _args;
        std::vector<Stmt *> _block;
        bool _isDeclaration;

    public:
        explicit FunDeclStmt(std::string name, ASTType retType, std::vector<Argument> args, std::vector<Stmt *> block, bool isDeclaration, AccessModifier access,
                             llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                           : _name(name), _retType(retType), _args(args), _block(block), _isDeclaration(isDeclaration), Stmt(NkFunDeclStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const marble::Node *node) {
            return node->GetKind() == NkFunDeclStmt;
        }

        std::string
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

        bool
        IsDeclaration() const {
            return _isDeclaration;
        }
    };
}
