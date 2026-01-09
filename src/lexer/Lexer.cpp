#include <onyx/basic/DiagKind.h>
#include <onyx/lexer/Token.h>
#include <onyx/lexer/Keywords.h>
#include <onyx/lexer/Lexer.h>

namespace onyx {
    Token
    Lexer::NextToken() {
        while (*_curPtr == ' ' || *_curPtr == '\n' || *_curPtr == '\t' || *_curPtr == '\r') {
            ++_curPtr;
        }

        const char *tokStart = _curPtr;
        if (*tokStart == '\0') {
            return Token(TkEof, "", llvm::SMLoc::getFromPointer(tokStart));
        }
        if (isalpha(*tokStart) || *tokStart == '_') {
            return tokenizeId(tokStart);
        }
        if (isdigit(*tokStart)) {
            return tokenizeNumLit(tokStart);
        }
        if (*tokStart == '\"') {
            return tokenizeStrLit(tokStart);
        }
        if (*tokStart == '\'') {
            return tokenizeCharLit(tokStart);
        }
        if (*tokStart == '/' && (*(tokStart + 1) == '/' || *(tokStart + 1) == '*')) {
            skipComments();
            return NextToken();
        }
        return tokenizeOp(tokStart);
    }

    Token
    Lexer::tokenizeId(const char *tokStart) {
        while (*_curPtr != '\0' && (isalnum(*_curPtr) || *_curPtr == '_')) {
            ++_curPtr;
        }
        llvm::StringRef text(tokStart, _curPtr - tokStart);
        if (keywords.find(text.data()) != keywords.end()) {
            return Token(keywords.at(text.data()), text, llvm::SMLoc::getFromPointer(tokStart));
        }
        return Token(TkId, text, llvm::SMLoc::getFromPointer(tokStart));
    }

    Token
    Lexer::tokenizeNumLit(const char *tokStart) {
        while (*_curPtr != '\0' && (isdigit(*_curPtr) || *_curPtr == '.')) {
            ++_curPtr;
        }
        llvm::StringRef text(tokStart, _curPtr - tokStart);
        // TODO: create lexing of other numerics literals
        return Token(TkI32Lit, text, llvm::SMLoc::getFromPointer(tokStart));
    }

    Token
    Lexer::tokenizeStrLit(const char *tokStart) {
        // TODO: create handling of escape-sequences
        ++_curPtr;  // skip "
        while (*_curPtr != '\0' && *_curPtr != '\"') {
            ++_curPtr;
        }
        ++_curPtr;  // skip "
        return Token(TkStrLit, llvm::StringRef(tokStart, _curPtr - tokStart), llvm::SMLoc::getFromPointer(tokStart));
    }

    Token
    Lexer::tokenizeCharLit(const char *tokStart) {
        // TODO: create handling of escape-sequences
        ++_curPtr;  // skip '
        while (*_curPtr != '\0' && *_curPtr != '\'') {
            ++_curPtr;
        }
        ++_curPtr;  // skip '
        // TODO: create error if length of literal is not equal 1
        return Token(TkCharLit, llvm::StringRef(tokStart, _curPtr - tokStart), llvm::SMLoc::getFromPointer(tokStart));
    }

    Token
    Lexer::tokenizeOp(const char *tokStart) {
        switch (*(_curPtr++)) {
            #define TOKEN(kind) Token(kind, llvm::StringRef(tokStart, _curPtr - tokStart), llvm::SMLoc::getFromPointer(tokStart))
            case ';':
                return TOKEN(TkSemi);
            case ',':
                return TOKEN(TkComma);
            case '.':
                return TOKEN(TkDot);
            case '(':
                return TOKEN(TkLParen);
            case ')':
                return TOKEN(TkRParen);
            case '{':
                return TOKEN(TkLBrace);
            case '}':
                return TOKEN(TkRBrace);
            case '[':
                return TOKEN(TkLBracket);
            case ']':
                return TOKEN(TkRBracket);
            case '@':
                return TOKEN(TkAt);
            case '~':
                return TOKEN(TkTilde);
            case '?':
                return TOKEN(TkQuestion);
            case ':':
                return TOKEN(TkColon);
            case '$':
                return TOKEN(TkDollar);
            case '=':
                if (*_curPtr == '=') {
                    ++_curPtr;
                    return TOKEN(TkEqEq);
                }
                return TOKEN(TkEq);
            case '!':
                if (*_curPtr == '=') {
                    ++_curPtr;
                    return TOKEN(TkNotEq);
                }
                return TOKEN(TkBang);
            case '>':
                if (*_curPtr == '=') {
                    ++_curPtr;
                    return TOKEN(TkGtEq);
                }
                return TOKEN(TkGt);
            case '<':
                if (*_curPtr == '=') {
                    ++_curPtr;
                    return TOKEN(TkLtEq);
                }
                return TOKEN(TkLt);
            case '&':
                if (*_curPtr == '&') {
                    ++_curPtr;
                    return TOKEN(TkLogAnd);
                }
                return TOKEN(TkAnd);
            case '|':
                if (*_curPtr == '|') {
                    ++_curPtr;
                    return TOKEN(TkLogOr);
                }
                return TOKEN(TkOr);
            case '+':
                if (*_curPtr == '=') {
                    ++_curPtr;
                    return TOKEN(TkPlusEq);
                }
                return TOKEN(TkPlus);
            case '-':
                if (*_curPtr == '=') {
                    ++_curPtr;
                    return TOKEN(TkMinusEq);
                }
                return TOKEN(TkMinus);
            case '*':
                if (*_curPtr == '=') {
                    ++_curPtr;
                    return TOKEN(TkStarEq);
                }
                return TOKEN(TkStar);
            case '/':
                if (*_curPtr == '=') {
                    ++_curPtr;
                    return TOKEN(TkSlashEq);
                }
                return TOKEN(TkSlash);
            case '%':
                if (*_curPtr == '=') {
                    ++_curPtr;
                    return TOKEN(TkPercentEq);
                }
                return TOKEN(TkPercent);
            case '^':
                return TOKEN(TkCarret);
            default:
                return TOKEN(TkUnknown);
        }
    }

    void
    Lexer::skipComments() {
        _curPtr += 2;
        bool isMultilineComment = *(_curPtr - 1) == '*';
        if (isMultilineComment) {
            while (*_curPtr != '\0' && (*_curPtr != '*' || *(_curPtr + 1) != '/')) {
                ++_curPtr;
            }
            _curPtr += 2;
        }
        else {
            while (*_curPtr != '\0' && *_curPtr != '\n') {
                ++_curPtr;
            }
        }
    }
}
