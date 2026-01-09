#pragma once
#include <onyx/basic/DiagnosticBuilder.h>

namespace onyx {
    class DiagnosticEngine {
        llvm::SourceMgr &_srcMgr;

    public:
        explicit DiagnosticEngine(llvm::SourceMgr &mgr) : _srcMgr(mgr) {}

        DiagnosticBuilder
        Report(llvm::SMLoc loc, DiagKind kind) {
            return DiagnosticBuilder(_srcMgr, loc, GetDiagInfo(kind));
        }
    };
}