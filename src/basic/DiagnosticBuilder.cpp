#include <onyx/basic/DiagnosticBuilder.h>

namespace onyx {
    DiagnosticBuilder::~DiagnosticBuilder() {
        if (!_isActive) return;

        std::string msg = _info.Format;
        for (size_t i = 0; i < _args.size(); ++i) {
            std::string placeholder = "%" + std::to_string(i);
            size_t pos = msg.find(placeholder);
            if (pos != std::string::npos) {
                msg.replace(pos, placeholder.length(), _args[i]);
            }
        }

        _srcMgr.PrintMessage(_loc, _info.Kind, msg, _ranges);
    }

    DiagnosticBuilder &
    DiagnosticBuilder::operator<<(llvm::StringRef s) {
        _args.push_back(s.str());
        return *this;
    }

    DiagnosticBuilder &
    DiagnosticBuilder::operator<<(llvm::SMRange r) {
        _ranges.push_back(r);
        return *this;
    }

    DiagnosticBuilder &
    DiagnosticBuilder::operator<<(long n) {
        _args.push_back(std::to_string(n));
        return *this;
    }

    DiagnosticBuilder &
    DiagnosticBuilder::operator<<(char c) {
        _args.push_back(std::string { c });
        return *this;
    }
}