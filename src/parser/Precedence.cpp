#include "onyx/Lexer/Token.h"
#include <onyx/Parser/Precedence.h>

namespace onyx {
    int
    GetTokPrecedence(TokenKind kind) {
        switch (kind) {
            case TkLogAnd:
            case TkLogOr:
                return PrecLogical;
            case TkGt:
            case TkGtEq:
            case TkLt:
            case TkLtEq:
                return PrecComparation;
            case TkEqEq:
            case TkNotEq:
                return PrecEquality;
            case TkAnd:
                return PrecBiwiseAnd;
            case TkCarret:
                return PrecBiwiseXor;
            case TkOr:
                return PrecBiwiseOr;
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