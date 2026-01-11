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
        ErrRedefinitionVar,
        ErrRedefinitionFun,
        ErrTypeMismatchNotNum,
        ErrTypeMismatchNotBool,
        ErrCannotCast,
        ErrUnsupportedTypeForOperator,
        ErrNotAllPathsReturnsValue,
        ErrFuntionCannotReturnValue,
        ErrFewArgs
    };
}