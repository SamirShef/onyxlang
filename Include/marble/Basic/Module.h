#include <marble/AST/Statements/FunDeclStmt.h>
#include <marble/AST/Statements/StructStmt.h>
#include <marble/AST/Statements/ImplStmt.h>
#include <marble/AST/Statements/TraitDeclStmt.h>
#include <llvm/IR/Module.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace marble {
    struct Module {
        private:
            std::string _name;
            std::string _fullPath;
            AccessModifier _access;

        public:
            std::vector<Stmt *> AST;
            std::unordered_map<std::string, FunDeclStmt *> Functions;
            std::unordered_map<std::string, StructStmt *> Structs;
            std::unordered_map<std::string, std::vector<ImplStmt *>> Implementations;
            std::unordered_map<std::string, TraitDeclStmt *> Traits;
            std::unordered_map<std::string, Module *> SubModules;
            std::vector<Module *> Imports;
            llvm::Module *Mod = nullptr;

            Module(std::string name, std::string fullPath, AccessModifier access) : _name(name), _fullPath(fullPath), _access(access) {}

            std::string
            GetName() const {
                return _name;
            }

            std::string
            GetFullPath() const {
                return _fullPath;
            }

            AccessModifier
            GetAccess() const {
                return _access;
            }
    };
}
