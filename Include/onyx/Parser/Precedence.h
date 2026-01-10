#pragma once
#include <onyx/Lexer/Token.h>

namespace onyx {
    enum Precedence {
        PrecLowest,
        PrecLogical,
        PrecComparation,
        PrecEquality,
        PrecBiwiseAnd,
        PrecBiwiseXor,
        PrecBiwiseOr,
        PrecSum,
        PrecProduct,
        PrecPrefix,
        PrecCall
    };
    
    int
    GetTokPrecedence(TokenKind kind);
}