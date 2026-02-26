#pragma once
#include <llvm/Support/Path.h>
#include <marble/Lexer/Lexer.h>
#include <marble/Parser/Parser.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <marble/Basic/Module.h>

namespace marble {
    class ModuleManager {
        inline static std::unordered_map<std::string, Module *> _mods;

    public:
        static Module *
        LoadModule(std::string path, llvm::SourceMgr &srcMgr, DiagnosticEngine &diag, AccessModifier access = AccessPub) {
            if (auto it = _mods.find(path); it != _mods.end()) {
                return it->second;
            }

            int last_dot_pos = path.find_last_of('.');
            if (last_dot_pos != std::string::npos && last_dot_pos != 0) { 
                path.erase(last_dot_pos);
            }
            auto bufferOrErr = llvm::MemoryBuffer::getFile(path + ".mr");
            
            if (std::error_code ec = bufferOrErr.getError()) {
                path += "/mod.mr";
                bufferOrErr = llvm::MemoryBuffer::getFile(path);
                
                if (std::error_code ec = bufferOrErr.getError()) {
                    return nullptr;
                }
            }
            unsigned bufferId = srcMgr.AddNewSourceBuffer(std::move(*bufferOrErr), llvm::SMLoc());
            Lexer lex(srcMgr, diag, bufferId);
            Parser parser(lex, diag);
            Module *mod = new Module(llvm::sys::path::stem(path).str(), access);
            mod->AST = parser.ParseAll();
            return mod;
        }
    };
}
