#pragma once
#include <marble/Basic/ASTType.h>

namespace marble {
    class Argument {
        const std::string _name;
        const ASTType _type;

    public:
        explicit Argument(std::string name, ASTType type) : _name(name), _type(type) {}

        std::string
        GetName() const {
            return _name;
        }

        ASTType
        GetType() const {
            return _type;
        }
    };
}