#pragma once
#include <marble/Basic/DiagnosticBuilder.h>

namespace marble {
    class DiagnosticEngine {
        llvm::SourceMgr &_srcMgr;
        bool _hasErrors = false;

    public:
        explicit DiagnosticEngine(llvm::SourceMgr &mgr) : _srcMgr(mgr) {}

        DiagnosticBuilder
        Report(llvm::SMLoc loc, DiagKind kind) {
            _hasErrors = true;
            return DiagnosticBuilder(_srcMgr, loc, GetDiagInfo(kind));
        }

        bool
        HasErrors() const {
            return _hasErrors;
        }

        void
        ResetErrors() {
            _hasErrors = false;
        }
    };
}