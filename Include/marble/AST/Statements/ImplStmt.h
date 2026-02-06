#pragma once
#include <marble/AST/Stmt.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace marble {
    class ImplStmt : public Stmt {
        std::string _traitName;
        std::string _structName;
        std::vector<Stmt *> _body;

    public:
        explicit ImplStmt(std::string traitName, std::string name, std::vector<Stmt *> body, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                        : _traitName(traitName), _structName(name), _body(body), Stmt(NkImplStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const marble::Node *node) {
            return node->GetKind() == NkImplStmt;
        }
        
        std::string
        GetTraitName() const {
            return _traitName;
        }

        std::string
        GetStructName() const {
            return _structName;
        }

        std::vector<Stmt *>
        GetBody() const {
            return _body;
        }
    };
}
