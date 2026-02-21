#include <marble/Basic/DiagKind.h>
#include <marble/Lexer/Token.h>
#include <marble/Lexer/Keywords.h>
#include <marble/Lexer/Lexer.h>

namespace marble {
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
        std::string text(llvm::StringRef(tokStart, _curPtr - tokStart).str());
        if (keywords.find(text) != keywords.end()) {
            return Token(keywords.at(text), text, llvm::SMLoc::getFromPointer(tokStart));
        }
        return Token(TkId, text, llvm::SMLoc::getFromPointer(tokStart));
    }

    Token
    Lexer::tokenizeNumLit(const char *tokStart) {
        bool hasDot = false;
        while (*_curPtr != '\0' && (isdigit(*_curPtr) || *_curPtr == '.' && !hasDot)) {
            if (*_curPtr == '.') {
                hasDot = true;
            }
            ++_curPtr;
        }
        std::string text(llvm::StringRef(tokStart, _curPtr - tokStart).str());
        #define TOKEN(kind) Token(kind, text, llvm::SMLoc::getFromPointer(tokStart))
        switch (tolower(*(_curPtr++))) {    // skip suffix (maybe not suffix)
            case 's':
                if (hasDot) {
                    _diag.Report(llvm::SMLoc::getFromPointer(_curPtr - 1), ErrIntegerSuffixForFloatingPoint)
                        << *(_curPtr - 1);
                }
                return TOKEN(TkI16Lit);
            case 'l':
                if (hasDot) {
                    _diag.Report(llvm::SMLoc::getFromPointer(_curPtr - 1), ErrIntegerSuffixForFloatingPoint)
                        << *(_curPtr - 1);
                }
                return TOKEN(TkI64Lit);
            case 'f':
                return TOKEN(TkF32Lit);
            case 'd':
                return TOKEN(TkF64Lit);
        }
        --_curPtr;                              // returned, because the character is not a suffix
        if (hasDot) {
            return TOKEN(TkF64Lit);
        }
        return TOKEN(TkI32Lit);
        #undef TOKEN
    }

    Token
    Lexer::tokenizeStrLit(const char *tokStart) {
        ++_curPtr;  // skip "
        std::string text;
        while (*_curPtr != '\0' && *_curPtr != '\"') {
            char c;
            if (*_curPtr == '\\') {
                c = getEscapeSecuence(++_curPtr);
            }
            else {
                c = *(_curPtr++);
            }
            text += c;
        }
        ++_curPtr;  // skip "
        return Token(TkStrLit, text, llvm::SMLoc::getFromPointer(tokStart));
    }

    Token
    Lexer::tokenizeCharLit(const char *tokStart) {
        ++_curPtr;  // skip '
        std::string text;
        while (*_curPtr != '\0' && *_curPtr != '\'') {
            char c;
            if (*_curPtr == '\\') {
                c = getEscapeSecuence(++_curPtr);
            }
            else {
                c = *(_curPtr++);
            }
            text += c;
        }
        ++_curPtr;  // skip '
        if (text.length() != 1) {
            _diag.Report(llvm::SMLoc::getFromPointer(tokStart), ErrCharLitLen)
                << llvm::SMRange(llvm::SMLoc::getFromPointer(tokStart), llvm::SMLoc::getFromPointer(_curPtr))
                << "'" + text + "'";
        }
        return Token(TkCharLit, text, llvm::SMLoc::getFromPointer(tokStart));
    }

    Token
    Lexer::tokenizeOp(const char *tokStart) {
        switch (*(_curPtr++)) {
            #define TOKEN(kind) Token(kind, std::string(tokStart, _curPtr - tokStart), llvm::SMLoc::getFromPointer(tokStart))
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

    const char
    Lexer::getEscapeSecuence(const char *tokStart) {
        switch (*(_curPtr++)) {
            case 'a':
                return '\a';
            case 'b':
                return '\b';
            case 'e':
                return '\e';
            case 'f':
                return '\f';
            case 'r':
                return '\r';
            case 'n':
                return '\n';
            case 't':
                return '\t';
            case 'v':
                return '\v';
            case '\\':
                return '\\';
            case '\'':
                return '\'';
            case '\"':
                return '\"';
            case '\?':
                return '\?';
            case '0':
                return '\0';
            default:
                --_curPtr;
                _diag.Report(llvm::SMLoc::getFromPointer(tokStart), ErrUnsupportedEscapeSequence)
                    << llvm::SMRange(llvm::SMLoc::getFromPointer(tokStart - 1), llvm::SMLoc::getFromPointer(tokStart))
                    << *tokStart;
                return *tokStart;
        }
    }
}
