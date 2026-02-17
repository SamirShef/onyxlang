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
        LoadModule(std::string fullPath, AccessModifier access) {
            if (_loadedModules.find(fullPath) != _loadedModules.end()) {
                return _loadedModules[fullPath];
            }

            Module *mod = new Module(fullPath, fullPath, access);
            _loadedModules[fullPath] = mod;

            llvm::SourceMgr srcMgr;
            auto bufferOrErr = llvm::MemoryBuffer::getFile(fullPath);
            if (std::error_code ec = bufferOrErr.getError()) {
                llvm::errs() << llvm::errs().RED << "Could not open file " << llvm::errs().RESET << '`' << fullPath << "`: " << ec.message() << '\n';
                return nullptr;
            }
            srcMgr.AddNewSourceBuffer(std::move(*bufferOrErr), llvm::SMLoc());

            Lexer lex(srcMgr, _diag);
            Parser parser(lex, _diag);
            mod->AST = parser.ParseAll();

            return mod;
        }

        const std::unordered_map<std::string, Module *> &
        GetLoadedModules() const {
            return _loadedModules;
        }
    };
}
