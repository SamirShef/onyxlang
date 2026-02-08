#pragma once
#include <marble/Basic/ASTType.h>

namespace marble {
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
        bool _isNil;

    public:
        explicit ASTVal(ASTType type, ASTValData data, bool isNil) : _type(type), _data(data), _isNil(isNil) {}
        
        ASTType
        GetType() const {
            return _type;
        }

        ASTValData
        GetData() const {
            return _data;
        }

        bool
        IsNil() const {
            return _isNil;
        }

        std::string
        ToString() const;

        double
        AsDouble() const;

        ASTVal
        Cast(ASTType type);

        static ASTVal
        GetVal(double val, ASTType type);

        static ASTVal
        GetDefaultByType(ASTType type);
    };
}
