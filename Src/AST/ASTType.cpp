#include <marble/Basic/ASTType.h>
#include <marble/Basic/Module.h>

namespace marble {
    std::string
    ASTType::ToString() const {
        std::string val;
        if (_isConst) {
            val += "const ";
        }
        for (int pd = _pointerDepth; pd > 0; --pd, val += "*");
        if (_mod) {
            val += _mod->ToString() + ".";
        }
        val += _val;
        return val;
    }
}
