#include <onyx/Basic/DiagKind.h>
#include <onyx/Basic/DiagInfo.h>

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
            case ErrUndeclaredVariable:
                return ERR("variable `%0` is undeclared");
            case ErrUndeclaredFuntion:
                return ERR("function `%0` is undeclared");
            case ErrUndeclaredStructure:
                return ERR("structure `%0` is undeclared");
            case ErrUndeclaredTrait:
                return ERR("trait `%0` is undeclared");
            case ErrUndeclaredField:
                return ERR("field `%0` is undeclared in structure `%1`");
            case ErrUndeclaredMethod:
                return ERR("method `%0` is undeclared in structure `%1`");
            case ErrRedefinitionVar:
                return ERR("redefinition the variable `%0`");
            case ErrRedefinitionFun:
                return ERR("redefinition the function `%0`");
            case ErrRedefinitionStruct:
                return ERR("redefinition the structure `%0`");
            case ErrRedefinitionField:
                return ERR("redefinition the field `%0`");
            case ErrRedefinitionMethod:
                return ERR("redefinition the method `%0`");
            case ErrRedefinitionTrait:
                return ERR("redefinition the trait `%0`");
            case ErrTypeMismatchNotNum:
                return ERR("expected number, but got `%0`");
            case ErrTypeMismatchNotBool:
                return ERR("expected bool, but got `%0`");
            case ErrCannotCast:
                return ERR("cannot implicitly cast `%0` to `%1`");
            case ErrUnsupportedTypeForOperator:
                return ERR("operator `%0` does not support types `%1` and `%2`");
            case ErrNotAllPathsReturnsValue:
                return ERR("not all paths return a value");
            case ErrFuntionCannotReturnValue:
                return ERR("function cannot return a value");
            case ErrMethodCannotReturnValue:
                return ERR("function cannot return a value");
            case ErrFewArgs:
                return ERR("function `%0` expected %1 argument(s), but received %2");
            case ErrAssignmentConst:
                return ERR("assigning a value to a constant");
            case ErrCannotBeHere:
                return ERR("this statement cannot be here");
            case ErrCannotHaveAccessBeHere:
                return ERR("this statement cannot have an access modifier be here");
            case ErrFieldInitialized:
                return ERR("field `%0` has already been initialized");
            case ErrAccessFromNonStruct:
                return ERR("accessing a member from non-struct");
            case ErrFieldIsPrivate:
                return ERR("field `%0` is private");
            case ErrMethodIsPrivate:
                return ERR("method `%0` is private");
            case ErrCannotDeclareHere:
                return ERR("cannot make declaration here (only in traits)");
            case ErrUndeclaredType:
                return ERR("type `%0` is undeclared");
            case ErrMethodNotInTrait:
                return ERR("method `%0` not contained in trait `%1`");
            case ErrCannotImplTraitMethod_RetTypeMismatch:
                return ERR("cannot imlement method `%0` from trait `%1`: expected return type `%2`, but received `%3`");
            case ErrCannotImplTraitMethod_FewArgs:
                return ERR("cannot imlement method `%0` from trait `%1: expected %2 argument(s), but received %3");
            case ErrCannotImplTraitMethod_ArgTypeMismatch:
                return ERR("cannot imlement method `%0` from trait `%1: argument `%2` expected type `%3`, but received `%4`");
            case ErrNotImplTraitMethod:
                return ERR("method `%0` from trait `%1` does not implemented");
        }
    }
}
