#pragma once
#include <onyx/lexer/Token.h>

namespace onyx {
    enum Precedence {
        PrecLowest      = 0,
        PrecEquality    = 10,
        PrecComparation = 20,
        PrecSum         = 30,
        PrecProduct     = 40,
        PrecPrefix      = 50,
        PrecCall        = 60
    };
    
    int
    GetTokPrecedence(TokenKind kind);
}