#include <marble/AST/Statements/FunDeclStmt.h>
#include <marble/AST/Statements/StructStmt.h>
#include <marble/AST/Statements/ImplStmt.h>
#include <marble/AST/Statements/TraitDeclStmt.h>
#include <marble/Basic/ASTVal.h>
#include <llvm/IR/Module.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace marble {
    struct Function {
        std::string Name;
        ASTType RetType;
        std::vector<Argument> Args;
        std::vector<Stmt *> Body;
        bool IsDeclaration;
        AccessModifier Access;
    };

    struct Field {
        std::string Name;
        std::optional<ASTVal> Val;
        ASTType Type;
        bool IsConst;
        AccessModifier Access;
        bool ManualInitialized;
    };
    
    struct Method {
        Function Fun;
        AccessModifier Access;
    };

    struct Trait {
        std::string Name;
        std::unordered_map<std::string, Method> Methods;
        AccessModifier Access;
    };

    struct Struct {
        std::string Name;
        std::unordered_map<std::string, Field> Fields;
        std::unordered_map<std::string, Method> Methods;
        std::unordered_map<std::string, Trait> TraitsImplements;
        AccessModifier Access;
    };

    struct Module {
        private:
            std::string _name;
            std::string _fullPath;
            AccessModifier _access;

        public:
            std::vector<Stmt *> AST;
            std::unordered_map<std::string, Function> Functions;
            std::unordered_map<std::string, Struct> Structs;
            std::unordered_map<std::string, std::vector<ImplStmt *>> Implementations;
            std::unordered_map<std::string, Trait> Traits;
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
