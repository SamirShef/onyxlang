#pragma once
#include <marble/Lexer/Token.h>
#include <unordered_map>

namespace marble {
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
        { "return", TkRet },
        { "if", TkIf },
        { "else", TkElse },
        { "for", TkFor },
        { "break", TkBreak },
        { "continue", TkContinue },
        { "struct", TkStruct },
        { "pub", TkPub },
        { "impl", TkImpl },
        { "trait", TkTrait },
        { "echo", TkEcho }
    };
}
