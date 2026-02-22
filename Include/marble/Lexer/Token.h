#pragma once
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/SMLoc.h>

namespace marble {
    enum TokenKind {
        TkId,               // identifier
        TkVar,              // keyword `var`
        TkConst,            // keyword `const`
        TkBool,             // keyword `bool`
        TkChar,             // keyword `char`
        TkI16,              // keyword `i16`
        TkI32,              // keyword `i32`
        TkI64,              // keyword `i64`
        TkF32,              // keyword `f32`
        TkF64,              // keyword `f64`
        TkNoth,             // keyword `noth`
        TkFun,              // keyword `fun`
        TkRet,              // keyword `return`
        TkIf,               // keyword `if`
        TkElse,             // keyword `else`
        TkFor,              // keyword `for`
        TkBreak,            // keyword `break`
        TkContinue,         // keyword `continue`
        TkStruct,           // keyword `struct`
        TkPub,              // keyword `pub`
        TkImpl,             // keyword `impl`
        TkTrait,            // keyword `trait`
        TkEcho,             // keyword `echo`
        TkNil,              // keyword `nil`
        TkNew,              // keyword `new`
        TkDel,              // keyword `del`
        TkImport,           // keyword `import`
        TkMod,              // keyword `mod`
        TkStatic,           // keyword `static`
        
        TkBoolLit,          // bool literal
        TkCharLit,          // character literal
        TkI16Lit,           // short literal
        TkI32Lit,           // int literal
        TkI64Lit,           // long literal
        TkF32Lit,           // float literal
        TkF64Lit,           // double literal
        TkStrLit,           // string literal
        
        TkSemi,             // `;`
        TkComma,            // `,`
        TkDot,              // `.`
        TkLParen,           // `(`
        TkRParen,           // `)`
        TkLBrace,           // `}`
        TkRBrace,           // `{`
        TkLBracket,         // `[`
        TkRBracket,         // `]`
        TkAt,               // `@`
        TkTilde,            // `~`
        TkQuestion,         // `?`
        TkColon,            // `:`
        TkDollar,           // `$`
        TkEq,               // `=`
        TkBang,             // `!`
        TkNotEq,            // `!=`
        TkAnd,              // '&`
        TkOr,               // `|`
        TkLogAnd,           // '&&`
        TkLogOr,            // `||`
        TkPlus,             // `+`
        TkMinus,            // `-`
        TkStar,             // `*`
        TkSlash,            // `/`
        TkPercent,          // `%`
        TkPlusEq,           // `+=`
        TkMinusEq,          // `-=`
        TkStarEq,           // `*=`
        TkSlashEq,          // `/=`
        TkPercentEq,        // `%=`
        TkEqEq,             // `==`
        TkLt,               // `<`
        TkGt,               // `>`
        TkLtEq,             // `<=`
        TkGtEq,             // `>=`
        TkCarret,           // `^`

        TkEof,
        TkUnknown,          // Unknown token, not expected by the lexer
    };
    
    class Token {
        TokenKind _kind;
        std::string _text;
        llvm::SMLoc _loc;

    public:
        explicit Token(TokenKind kind, std::string text, llvm::SMLoc loc) : _kind(kind), _text(text), _loc(loc) {}

        const TokenKind
        GetKind() const {
            return _kind;
        }

        const std::string
        GetText() const {
            return _text;
        }

        const llvm::SMLoc
        GetLoc() const {
            return _loc;
        }

        bool
        Is(TokenKind kind) {
            return _kind == kind;
        }
    };
}
