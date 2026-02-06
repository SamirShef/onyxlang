#pragma once
#include <marble/Lexer/Token.h>

namespace marble {
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