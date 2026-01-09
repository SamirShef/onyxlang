#include <onyx/basic/DiagKind.h>
#include <onyx/basic/DiagInfo.h>

namespace onyx {
    DiagInfo
    GetDiagInfo(DiagKind kind) {
        #define ERR(msg) DiagInfo(llvm::SourceMgr::DiagKind::DK_Error, msg)
        switch (kind) {
            case ErrIntegerSuffixForFloatingPoint:
                return ERR("using the `%0` suffix for floating point numeric literal");
            case ErrUnsupportedEscapeSequence:
                return ERR("escape-sequence `\\%0` is not supported");
            case ErrCharLitLen:
                return ERR("character literal `%0` must have a length equal to 1");
            case ErrExpectedExpr:
                return ERR("expected expression, but got `%0`");
            case ErrExpectedStmt:
                return ERR("expected statement, but got `%0`");
            case ErrExpectedToken:
                return ERR("expected `%0`, but got `%1`");
            case ErrExpectedId:
                return ERR("expected identifier, but got `%0`");
            case ErrExpectedType:
                return ERR("expected type, but got `%0`");
        }
    }
}