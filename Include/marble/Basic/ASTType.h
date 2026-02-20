#pragma once
#include <string>

namespace marble {
    enum class ASTTypeKind {
        Bool,
        Char,
        I16,
        I32,
        I64,
        F32,
        F64,
        Struct,
        Trait,
        Noth,
        Nil,
        Mod,
    };
    
    class ASTType {
        ASTTypeKind _kind;
        std::string _val;
        bool _isConst;
        unsigned char _pointerDepth;

    public:
        ASTType() = default;

        explicit ASTType(ASTTypeKind kind, std::string val, bool isConst, unsigned char pointerDepth)
                       : _kind(kind), _val(val), _isConst(isConst), _pointerDepth(pointerDepth) {}

        bool
        operator==(ASTType &other) {
            return _kind == other._kind && _val == other._val && _pointerDepth == other._pointerDepth;
        }

        bool
        operator!=(ASTType &other) {
            return !(*this == other);
        }
        
        bool
        operator==(const ASTType &other) {
            return _kind == other._kind && _val == other._val && _pointerDepth == other._pointerDepth;
        }

        bool
        operator!=(const ASTType &other) {
            return !(*this == other);
        }

        ASTTypeKind
        GetTypeKind() const {
            return _kind;
        }

        std::string
        GetVal() const {
            return _val;
        }

        bool
        IsConst() const {
            return _isConst;
        }

        bool
        IsPointer() const {
            return _pointerDepth;
        }

        unsigned char
        GetPointerDepth() const {
            return _pointerDepth;
        }

        ASTType
        Deref() {
            --_pointerDepth;
            return *this;
        }

        ASTType
        Ref() {
            ++_pointerDepth;
            return *this;
        }

        std::string
        ToString() const {
            std::string val;
            if (_isConst) {
                val += "const ";
            }
            for (int pd = _pointerDepth; pd > 0; --pd, val += "*");
            val += _val;
            return val;
        }

        static ASTType
        GetNothType() {
            return ASTType(ASTTypeKind::Noth, "noth", false, 0);
        }
        
        static ASTType
        GetCommon(ASTType lhs, ASTType rhs) {
            if (lhs == rhs) {
                return lhs;
            }
            if (lhs.IsPointer() && rhs.GetTypeKind() >= ASTTypeKind::Char && rhs.GetTypeKind() <= ASTTypeKind::I64) {
                return lhs;
            }
            if (rhs.IsPointer() && lhs.GetTypeKind() >= ASTTypeKind::Char && lhs.GetTypeKind() <= ASTTypeKind::I64) {
                return rhs;
            }
            if (lhs.GetTypeKind() >= ASTTypeKind::Char && lhs.GetTypeKind() <= ASTTypeKind::F64 &&
                rhs.GetTypeKind() >= ASTTypeKind::Char && rhs.GetTypeKind() <= ASTTypeKind::F64) {
                return lhs.GetTypeKind() > rhs.GetTypeKind() ? lhs : rhs;
            }
            return GetNothType();
        }

        static bool
        HasCommon(ASTType lhs, ASTType rhs) {
            if (lhs == rhs) {
                return true;
            }
            if (lhs.IsPointer() && rhs.GetTypeKind() >= ASTTypeKind::Char && rhs.GetTypeKind() <= ASTTypeKind::I64) {
                return true;
            }
            if (rhs.IsPointer() && lhs.GetTypeKind() >= ASTTypeKind::Char && lhs.GetTypeKind() <= ASTTypeKind::I64) {
                return true;
            }
            if (lhs.GetTypeKind() >= ASTTypeKind::Char && lhs.GetTypeKind() <= ASTTypeKind::F64 &&
                rhs.GetTypeKind() >= ASTTypeKind::Char && rhs.GetTypeKind() <= ASTTypeKind::F64) {
                return true;
            }
            return false;
        }
    };
}
