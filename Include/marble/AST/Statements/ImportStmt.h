#pragma once
#include <marble/AST/Stmt.h>
#include <string>

namespace marble {
    class ImportStmt : public Stmt {
        std::string _path;
        bool _isLocalImport;

    public:
        explicit ImportStmt(std::string path, bool isLocalimport, AccessModifier access, llvm::SMLoc startLoc, llvm::SMLoc endLoc)
                             : _path(path), _isLocalImport(isLocalimport), Stmt(NkImportStmt, access, startLoc, endLoc) {}

        constexpr static bool
        classof(const Node *node) {
            return node->GetKind() == NkImportStmt;
        }
        
        std::string
        GetPath() const {
            return _path;
        }

        bool
        IsLocalImport() const {
            return _isLocalImport;
        }
    };
}
