#pragma once

namespace onyx {
    enum class ASTTypeKind {
        Bool,
        Char,
        I16,
        I32,
        I64,
        F32,
        F64,
        Noth
    };
    
    class ASTType {
        ASTTypeKind _kind;
        bool _isConst;

    public:
        explicit ASTType(ASTTypeKind kind, bool isConst) : _kind(kind), _isConst(isConst) {}

        ASTTypeKind
        GetTypeKind() const {
            return _kind;
        }

        bool
        IsConst() const {
            return _isConst;
        }
    };
}