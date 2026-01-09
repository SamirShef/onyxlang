#pragma once
#include <onyx/basic/DiagInfo.h>

namespace onyx {
    class DiagnosticBuilder {
        llvm::SourceMgr &_mgr;
        llvm::SMLoc _loc;
        DiagInfo _info;
        std::vector<std::string> _args;
        bool _isActive = true;

    public:
        explicit DiagnosticBuilder(llvm::SourceMgr &mgr, llvm::SMLoc loc, DiagInfo info) : _mgr(mgr), _loc(loc), _info(info) {}

        DiagnosticBuilder(DiagnosticBuilder &&other) : _mgr(other._mgr), _loc(other._loc), _info(other._info), _args(other._args), _isActive(other._isActive) {
            other._isActive = false;
        }

        ~DiagnosticBuilder();
        
        DiagnosticBuilder &
        operator<<(llvm::StringRef s);

        DiagnosticBuilder &
        operator<<(long n);

        DiagnosticBuilder &
        operator<<(char c);
    };
}