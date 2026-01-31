#pragma once
#include <onyx/AST/Stmt.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace onyx {
    class TraitDeclStmt : public Stmt {
        llvm::StringRef _name;
        std::vector<Stmt *> _body;

    public:
        explicit TraitDeclStmt(llvm::StringRef name, std::vector<Stmt *> body, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                             : _name(name), _body(body), Stmt(NkTraitDeclStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const onyx::Node *node) {
            return node->GetKind() == NkTraitDeclStmt;
        }
        
        llvm::StringRef
        GetName() const {
            return _name;
        }

        std::vector<Stmt *>
        GetBody() const {
            return _body;
        }
    };
}
