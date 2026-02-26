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
    };

    struct Function {
        std::string Name;
        ASTType RetType;
        std::vector<Argument> Args;
        std::vector<Stmt *> Body;
        bool IsDeclaration;
        AccessModifier Access;
    };

    struct Method {
        Function Fun;
        bool IsConst;
        AccessModifier Access;
    };

    struct Trait {
        std::string Name;
        std::unordered_map<std::string, Method> Methods;
    };

    struct Field {
        std::string Name;
        std::optional<ASTVal> Val;
        ASTType Type;
        bool IsConst;
        AccessModifier Access;
        bool ManualInitialized;
    };

    struct Struct {
        std::string Name;
        std::unordered_map<std::string, Field> Fields;
        std::unordered_map<std::string, Method> Methods;
        std::unordered_map<std::string, Trait> TraitsImplements;
    };
}
