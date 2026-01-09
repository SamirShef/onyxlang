#include "onyx/lexer/Token.h"
#include <onyx/parser/Precedence.h>

namespace onyx {
    int
    GetTokPrecedence(TokenKind kind) {
        switch (kind) {
            case TkEqEq:
            case TkNotEq:
                return PrecEquality;
            case TkGt:
            case TkGtEq:
            case TkLt:
            case TkLtEq:
                return PrecComparation;
            case TkPlus:
            case TkMinus:
                return PrecSum;
            case TkStar:
            case TkSlash:
            case TkPercent:
                return PrecProduct;
            case TkLParen:
                return PrecCall;
            default:
                return PrecLowest;
        }
    }
}