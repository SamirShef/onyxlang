#pragma once
#include <onyx/Basic/DiagKind.h>
#include <llvm/Support/SourceMgr.h>

namespace onyx {
    struct DiagInfo {
        const llvm::SourceMgr::DiagKind Kind;
        const char * const Format;

        explicit DiagInfo(llvm::SourceMgr::DiagKind kind, const char *format) : Kind(kind), Format(format) {}
    };

    DiagInfo
    GetDiagInfo(DiagKind kind);
}