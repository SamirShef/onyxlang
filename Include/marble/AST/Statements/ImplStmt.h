#pragma once
#include <marble/Basic/ASTType.h>
#include <marble/AST/Stmt.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace marble {
    class ImplStmt : public Stmt {
        ASTType _traitType;
        ASTType _structType;
        std::vector<Stmt *> _body;

    public:
        explicit ImplStmt(ASTType traitType, ASTType structType, std::vector<Stmt *> body, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                        : _traitType(traitType), _structType(structType), _body(body), Stmt(NkImplStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkImplStmt;
        }
        
        ASTType &
        GetTraitType() {
            return _traitType;
        }

        ASTType &
        GetStructType() {
            return _structType;
        }

        std::vector<Stmt *>
        GetBody() const {
            return _body;
        }
    };
}
