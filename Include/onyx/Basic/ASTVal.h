#pragma once
#include <onyx/Basic/ASTType.h>

namespace onyx {
    union ASTValData {
        bool    boolVal;
        char    charVal;
        short   i16Val;
        int     i32Val;
        long    i64Val;
        float   f32Val;
        double  f64Val;
    };

    class ASTVal {
        ASTType _type;
        ASTValData _data;

    public:
        explicit ASTVal(ASTType type, ASTValData data) : _type(type), _data(data) {}

        ASTType
        GetType() const {
            return _type;
        }

        ASTValData
        GetData() const {
            return _data;
        }

        std::string
        ToString() const {
            switch (_type.GetTypeKind()) {
                #define TO_STR(field) std::to_string(_data.field)
                case ASTTypeKind::Bool:
                    return TO_STR(boolVal);
                case ASTTypeKind::Char:
                    return TO_STR(charVal);
                case ASTTypeKind::I16:
                    return TO_STR(i16Val);
                case ASTTypeKind::I32:
                    return TO_STR(i32Val);
                case ASTTypeKind::I64:
                    return TO_STR(i64Val);
                case ASTTypeKind::F32:
                    return TO_STR(f32Val);
                case ASTTypeKind::F64:
                    return TO_STR(f64Val);
                case ASTTypeKind::Noth:
                    return "<noth>";
            }
        }
    };
}