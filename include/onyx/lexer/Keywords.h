#pragma once
#include <onyx/lexer/Token.h>
#include <unordered_map>

namespace onyx {
    static std::unordered_map<std::string, TokenKind> keywords {
        { "false", TkBoolLit },
        { "true", TkBoolLit },
        { "var", TkVar },
        { "const", TkConst },
        { "bool", TkBool },
        { "char", TkChar },
        { "i16", TkI16 },
        { "i32", TkI32 },
        { "i64", TkI64 },
        { "f32", TkF32 },
        { "f64", TkF64 },
        { "noth", TkNoth },
        { "fun", TkFun },
        { "ret", TkRet },
        { "if", TkIf },
        { "else", TkElse },
        { "for", TkFor },
        { "break", TkBreak },
        { "continue", TkContinue },
        { "struct", TkStruct },
        { "pub", TkPub },
    };
}