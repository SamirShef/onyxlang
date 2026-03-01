#pragma once
#include <marble/AST/Argument.h>
#include <marble/AST/Stmt.h>
#include <marble/Basic/ASTType.h>
#include <marble/Basic/ASTVal.h>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace marble {
    struct Variable {
        std::string Name;
        ASTType Type;
        std::optional<ASTVal> Val;
        bool IsConst;
        AccessModifier Access;
        Module *Parent = nullptr;
    };

    struct Function {
        std::string Name;
        ASTType RetType;
        std::vector<Argument> Args;
        std::vector<Stmt *> Body;
        bool IsDeclaration;
        AccessModifier Access;
        Module *Parent = nullptr;
    };

    struct Method {
        Function Fun;
        bool IsConst;
        AccessModifier Access;
        bool IsStatic;
    };

    struct Trait {
        std::string Name;
        std::unordered_map<std::string, Method> Methods;
        AccessModifier Access;
        Module *Parent = nullptr;
    };

    struct Field {
        std::string Name;
        std::optional<ASTVal> Val;
        ASTType Type;
        bool IsConst;
        AccessModifier Access;
        bool ManualInitialized;
        bool IsStatic;
    };

    struct Struct {
        std::string Name;
        std::unordered_map<std::string, Field> Fields;
        std::unordered_map<std::string, Method> Methods;
        std::unordered_map<std::string, Trait> TraitsImplements;
        AccessModifier Access;
        Module *Parent = nullptr;
    };
}
