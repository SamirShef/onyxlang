#pragma once
#include <marble/AST/Stmt.h>
#include <string>
#include <vector>

namespace marble {
    class ModDeclStmt : public Stmt {
        std::string _name;
        std::vector<Stmt *> _body;

    public:
        explicit ModDeclStmt(std::string name, std::vector<Stmt *> body, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                          : _name(name), _body(body), Stmt(NkModDeclStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkModDeclStmt;
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
