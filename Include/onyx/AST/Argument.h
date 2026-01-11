#pragma once
#include <onyx/Basic/ASTType.h>

namespace onyx {
    class Argument {
        const llvm::StringRef _name;
        const ASTType _type;

    public:
        explicit Argument(llvm::StringRef name, ASTType type) : _name(name), _type(type) {}

        llvm::StringRef
        GetName() const {
            return _name;
        }

        ASTType
        GetType() const {
            return _type;
        }
    };
}