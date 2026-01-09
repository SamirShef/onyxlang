#include <onyx/basic/DiagKind.h>
#include <onyx/basic/DiagInfo.h>

namespace onyx {
    DiagInfo
    GetDiagInfo(DiagKind kind) {
        switch (kind) {
            case ErrExpectedSemi:
                return DiagInfo(llvm::SourceMgr::DiagKind::DK_Error, "expected `;`, but got `%0`");
        }
    }
}