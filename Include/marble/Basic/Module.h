#pragma once
#include <marble/Basic/Symbols.h>
#include <marble/AST/Stmt.h>
#include <unordered_map>
#include <string>
#include <vector>

namespace marble {
    struct Module {
        std::string Name;
        AccessModifier Access;
        std::vector<Stmt *> AST;
        Module *Parent = nullptr;
        std::unordered_map<std::string, Variable> Variables;
        std::unordered_map<std::string, Function> Functions;
        std::unordered_map<std::string, Struct> Structures;
        std::unordered_map<std::string, Trait> Traits;
        std::unordered_map<std::string, Module *> Submodules;
        std::unordered_map<std::string, Module *> Imports;
        
        explicit Module(std::string name, AccessModifier access) : Name(name), Access(access) {}

        Struct *
        FindStruct(const std::string &name) {
            if (auto it = Structures.find(name); it != Structures.end()) {
                return &it->second;
            }
            for (auto const &[_, mod] : Imports) {
                if (auto *s = mod->FindStruct(name)) {
                    return s;
                }
            }
            return nullptr;
        }

        Trait *
        FindTrait(const std::string &name) {
            if (auto it = Traits.find(name); it != Traits.end()) {
                return &it->second;
            }
            for (auto const &[_, mod] : Imports) {
                if (auto *t = mod->FindTrait(name)) {
                    return t;
                }
            }
            return nullptr;
        }

        Function *
        FindFunction(const std::string &name) {
            if (auto it = Functions.find(name); it != Functions.end()) {
                return &it->second;
            }
            for (auto const &[_, mod] : Imports) {
                if (auto *f = mod->FindFunction(name)) {
                    return f;
                }
            }
            return nullptr;
        }

        Variable *
        FindGlobalVar(const std::string &name) {
            if (auto it = Variables.find(name); it != Variables.end()) {
                return &it->second;
            }
            for (auto const &[_, mod] : Imports) {
                if (auto *v = mod->FindGlobalVar(name)) {
                    return v;
                }
            }
            return nullptr;
        }
    };
}
