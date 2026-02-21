#pragma once
#include <marble/Lexer/Lexer.h>
#include <marble/Parser/Parser.h>
#include <marble/Basic/DiagnosticEngine.h>
#include <marble/Basic/Module.h>
#include <string>
#include <unordered_map>

namespace marble {
    class ModuleManager {
        std::unordered_map<std::string, Module *> _loadedModules;
        DiagnosticEngine &_diag;

    public:
        ModuleManager(DiagnosticEngine &diag) : _diag(diag) {}

        Module *
        LoadModule(std::string fullPath, AccessModifier access, llvm::SourceMgr &srcMgr) {
            if (_loadedModules.find(fullPath) != _loadedModules.end()) {
                return _loadedModules[fullPath];
            }
            auto bufferOrErr = llvm::MemoryBuffer::getFile(fullPath);
            if (std::error_code ec = bufferOrErr.getError()) {
                return nullptr;
            }

            Module *mod = new Module(fullPath, fullPath, access);
            _loadedModules[fullPath] = mod;

            std::unique_ptr<llvm::MemoryBuffer> buffer = std::move(*bufferOrErr);
            const char *bufferStart = buffer->getBufferStart();
            const char *bufferEnd = buffer->getBufferEnd();
            unsigned bufferID = srcMgr.AddNewSourceBuffer(std::move(buffer), llvm::SMLoc());
            llvm::StringRef srcMgrBuffer = srcMgr.getMemoryBuffer(bufferID)->getBuffer();
            
            Lexer lex(_diag, srcMgr, bufferID);
            Parser parser(lex, _diag, srcMgr, *this);
            mod->AST = parser.ParseAll();

            return mod;
        }

        const std::unordered_map<std::string, Module *> &
        GetLoadedModules() const {
            return _loadedModules;
        }
    };
}
