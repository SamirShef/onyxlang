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
