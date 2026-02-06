#include <marble/Basic/DiagnosticBuilder.h>

namespace marble {
    DiagnosticBuilder::~DiagnosticBuilder() {
        if (!_isActive) {
            return;
        }

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
    DiagnosticBuilder::operator<<(std::string s) {
        _args.push_back(s);
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
        _args.push_back(std::string(1, c));
        return *this;
    }

    DiagnosticBuilder &
    DiagnosticBuilder::operator<<(size_t s) {
        _args.push_back(std::to_string(s));
        return *this;
    }
}