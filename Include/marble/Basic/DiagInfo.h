#pragma once
#include <marble/Basic/DiagKind.h>
#include <llvm/Support/SourceMgr.h>

namespace marble {
    struct DiagInfo {
        const llvm::SourceMgr::DiagKind Kind;
        const char *Format;

        explicit DiagInfo(llvm::SourceMgr::DiagKind kind, const char *format) : Kind(kind), Format(format) {}
    };

    DiagInfo
    GetDiagInfo(DiagKind kind);
}