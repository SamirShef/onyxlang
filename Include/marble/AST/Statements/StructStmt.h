#pragma once
#include <marble/AST/Stmt.h>
#include <llvm/ADT/StringRef.h>
#include <vector>

namespace marble {
    class StructStmt : public Stmt {
        std::string _name;
        std::vector<Stmt *> _body;

    public:
        explicit StructStmt(std::string name, std::vector<Stmt *> body, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                          : _name(name), _body(body), Stmt(NkStructStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const marble::Node *node) {
            return node->GetKind() == NkStructStmt;
        }
        
        std::string
        GetName() const {
            return _name;
        }

        std::vector<Stmt *>
        GetBody() const {
            return _body;
        }
    };
}