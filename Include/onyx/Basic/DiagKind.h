#pragma once

namespace onyx {
    enum DiagKind {
        // errors, warnings and notes
        ErrIntegerSuffixForFloatingPoint,
        ErrUnsupportedEscapeSequence,
        ErrCharLitLen,
        ErrExpectedExpr,
        ErrExpectedStmt,
        ErrExpectedToken,
        ErrExpectedId,
        ErrExpectedType,
        ErrUndeclaredVariable,
        ErrUndeclaredFuntion,
        ErrUndeclaredStructure,
        ErrUndeclaredField,
        ErrRedefinitionVar,
        ErrRedefinitionFun,
        ErrRedefinitionStruct,
        ErrRedefinitionField,
        ErrTypeMismatchNotNum,
        ErrTypeMismatchNotBool,
        ErrCannotCast,
        ErrUnsupportedTypeForOperator,
        ErrNotAllPathsReturnsValue,
        ErrFuntionCannotReturnValue,
        ErrFewArgs,
        ErrAssignmentConst,
        ErrCannotBeHere,
        ErrCannotHaveAccessBeHere,
        ErrFieldInitialized,
        ErrAccessFromNonStruct,
        ErrFieldIsPrivate
    };
}
