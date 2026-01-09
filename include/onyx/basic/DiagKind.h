#pragma once

namespace onyx {
    enum DiagKind {
        // errors, warnings and notes
        ErrIntegerSuffixForFloatingPoint,
        ErrUnsupportedEscapeSequence,
        ErrCharLitLen,
        ErrExpectedSemi,
    };
}