#pragma once
#include <onyx/basic/DiagInfo.h>

namespace onyx {
    class DiagnosticBuilder {
        llvm::SourceMgr &_srcMgr;
        llvm::SMLoc _loc;
        DiagInfo _info;
        std::vector<std::string> _args;
        std::vector<llvm::SMRange> _ranges;
        bool _isActive = true;

    public:
        explicit DiagnosticBuilder(llvm::SourceMgr &mgr, llvm::SMLoc loc, DiagInfo info) : _srcMgr(mgr), _loc(loc), _info(info) {}

        DiagnosticBuilder(DiagnosticBuilder &&other) : _srcMgr(other._srcMgr), _loc(other._loc), _info(other._info), _args(other._args), _ranges(other._ranges),
                                                       _isActive(other._isActive) {
            other._isActive = false;
        }

        ~DiagnosticBuilder();
        
        DiagnosticBuilder &
        operator<<(llvm::StringRef s);

        DiagnosticBuilder &
        operator<<(llvm::SMRange r);

        DiagnosticBuilder &
        operator<<(long n);

        DiagnosticBuilder &
        operator<<(char c);
    };
}