#pragma once
#include <marble/AST/Argument.h>
#include <marble/AST/Stmt.h>
#include <marble/Basic/ASTType.h>
#include <vector>

namespace marble {
    class FunDeclStmt : public Stmt {
        std::string _name;
        ASTType _retType;
        std::vector<Argument> _args;
        std::vector<Stmt *> _block;
        bool _isDeclaration;
        bool _isStatic;

    public:
        explicit FunDeclStmt(std::string name, ASTType retType, std::vector<Argument> args, std::vector<Stmt *> block, bool isDeclaration, bool isStatic, AccessModifier access,
                             llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                           : _name(name), _retType(retType), _args(args), _block(block), _isDeclaration(isDeclaration), _isStatic(isStatic),
                             Stmt(NkFunDeclStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
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

        void
        SetRetType(ASTType type) {
            _retType = type;
        }

        std::vector<Argument> &
        GetArgs() {
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

        bool
        IsStatic() const {
            return _isStatic;
        }
    };
}
