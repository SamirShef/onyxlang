#pragma once
#include <onyx/AST/Stmt.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace onyx {
    class ImplStmt : public Stmt {
        llvm::StringRef _traitName;
        llvm::StringRef _structName;
        std::vector<Stmt *> _body;

    public:
        explicit ImplStmt(llvm::StringRef traitName, llvm::StringRef name, std::vector<Stmt *> body, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                        : _traitName(traitName), _structName(name), _body(body), Stmt(NkImplStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const onyx::Node *node) {
            return node->GetKind() == NkImplStmt;
        }
        
        llvm::StringRef
        GetTraitName() const {
            return _traitName;
        }

        llvm::StringRef
        GetStructName() const {
            return _structName;
        }

        std::vector<Stmt *>
        GetBody() const {
            return _body;
        }
    };
}
