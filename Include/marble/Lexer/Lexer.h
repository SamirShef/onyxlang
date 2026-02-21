#pragma once
#include <marble/Basic/DiagnosticEngine.h>
#include <marble/Lexer/Token.h>
#include <llvm/Support/SourceMgr.h>

namespace marble {
    class Lexer {
        DiagnosticEngine &_diag;
        llvm::SourceMgr &_srcMgr;
        const char *_curPtr;

    public:
        explicit Lexer(DiagnosticEngine &diag, llvm::SourceMgr &srcMgr, unsigned bufferID) : _diag(diag), _srcMgr(srcMgr) {
            llvm::StringRef buffer = srcMgr.getMemoryBuffer(bufferID)->getBuffer();
            _curPtr = buffer.begin();
        }


        Token
        NextToken();
    
    private:
        Token
        tokenizeId(const char *tokStart);

        Token
        tokenizeNumLit(const char *tokStart);

        Token
        tokenizeStrLit(const char *tokStart);

        Token
        tokenizeCharLit(const char *tokStart);

        Token
        tokenizeOp(const char *tokStart);

        void
        skipComments();

        const char
        getEscapeSecuence(const char *tokStart);
    };
}
