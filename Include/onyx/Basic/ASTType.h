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
        Noth
    };
    
    class ASTType {
        ASTTypeKind _kind;
        llvm::StringRef _val;
        bool _isConst;

    public:
        explicit ASTType(ASTTypeKind kind, llvm::StringRef val, bool isConst) : _kind(kind), _val(val), _isConst(isConst) {}

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
    };
}