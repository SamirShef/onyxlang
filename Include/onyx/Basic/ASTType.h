#pragma once
#include <llvm/ADT/StringRef.h>

namespace onyx {
    enum class ASTTypeKind {
        Bool,
        Char,
        I16,
        I32,
        I64,
        F32,
        F64,
        Struct,
        Noth
    };
    
    class ASTType {
        ASTTypeKind _kind;
        llvm::StringRef _val;
        bool _isConst;

    public:
        explicit ASTType(ASTTypeKind kind, llvm::StringRef val, bool isConst) : _kind(kind), _val(val), _isConst(isConst) {}

        bool
        operator==(ASTType &other) {
            return _kind == other._kind && _val == other._val;
        }
        
        ASTTypeKind
        GetTypeKind() const {
            return _kind;
        }

        llvm::StringRef
        GetVal() const {
            return _val;
        }

        bool
        IsConst() const {
            return _isConst;
        }

        std::string
        ToString() const {
            std::string val;
            if (_isConst) {
                val += "const ";
            }
            val += _val;
            return val;
        }

        static ASTType
        GetNothType() {
            return ASTType(ASTTypeKind::Noth, "noth", false);
        }
        
        static ASTType
        GetCommon(ASTType lhs, ASTType rhs) {
            if (lhs == rhs) {
                return lhs;
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
            if (lhs.GetTypeKind() >= ASTTypeKind::Char && lhs.GetTypeKind() <= ASTTypeKind::F64 &&
                rhs.GetTypeKind() >= ASTTypeKind::Char && rhs.GetTypeKind() <= ASTTypeKind::F64) {
                return true;
            }
            return false;
        }
    };
}