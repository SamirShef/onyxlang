#pragma once
#include <onyx/AST/Stmt.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace onyx {
    class StructStmt : public Stmt {
        llvm::StringRef _name;
        std::vector<Stmt *> _body;

    public:
        explicit StructStmt(llvm::StringRef name, std::vector<Stmt *> body, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                          : _name(name), _body(body), Stmt(NkStructStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const onyx::Node *node) {
            return node->GetKind() == NkStructStmt;
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