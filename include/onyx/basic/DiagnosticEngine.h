#pragma once
#include <onyx/basic/DiagnosticBuilder.h>

namespace onyx {
    class DiagnosticEngine {
        llvm::SourceMgr &_mgr;

    public:
        explicit DiagnosticEngine(llvm::SourceMgr &mgr) : _mgr(mgr) {}

        DiagnosticBuilder
        Report(llvm::SMLoc loc, DiagKind kind) {
            return DiagnosticBuilder(_mgr, loc, GetDiagInfo(kind));
        }
    };
}