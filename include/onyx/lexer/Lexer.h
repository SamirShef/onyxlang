#pragma once
#include <onyx/lexer/Token.h>
#include <llvm/Support/SourceMgr.h>

namespace onyx {
    class Lexer {
        llvm::SourceMgr &_srcMgr;
        unsigned _curBuf;
        const char *_bufStart;
        const char *_curPtr;

    public:
        explicit Lexer(llvm::SourceMgr &mgr) : _srcMgr(mgr) {
            _curBuf = _srcMgr.getMainFileID();
            auto *buf = _srcMgr.getMemoryBuffer(_curBuf);
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
    };
}