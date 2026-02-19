#pragma once
#include <marble/Basic/DiagnosticEngine.h>
#include <marble/Lexer/Token.h>
#include <llvm/Support/SourceMgr.h>

namespace marble {
    class Lexer {
        llvm::SourceMgr &_srcMgr;
        DiagnosticEngine &_diag;
        const char *_bufStart;
        const char *_curPtr;

    public:
        explicit Lexer(llvm::SourceMgr &mgr, DiagnosticEngine &diag, unsigned bufferID) : _srcMgr(mgr), _diag(diag) {
            auto *buf = _srcMgr.getMemoryBuffer(bufferID);
            _bufStart = buf->getBufferStart();
            _curPtr = _bufStart;
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
