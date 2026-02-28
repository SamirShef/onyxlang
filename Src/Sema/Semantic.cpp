#include <marble/Basic/ModuleManager.h>
#include <marble/Sema/Semantic.h>
#include <cmath>

static bool isConstMethod = true;

static std::vector<std::string>
splitString(std::string src, char separator);

namespace marble {
    static std::unordered_map<ASTTypeKind, std::vector<ASTTypeKind>> implicitlyCastAllowed {
        { ASTTypeKind::Char, { ASTTypeKind::I16, ASTTypeKind::I32, ASTTypeKind::I64, ASTTypeKind::F32, ASTTypeKind::F64 } },
        { ASTTypeKind::I16,  { ASTTypeKind::I32, ASTTypeKind::I64, ASTTypeKind::F32, ASTTypeKind::F64                   } },
        { ASTTypeKind::I32,  { ASTTypeKind::I64, ASTTypeKind::F32, ASTTypeKind::F64                                     } },
        { ASTTypeKind::I64,  { ASTTypeKind::F32, ASTTypeKind::F64                                                       } },
        { ASTTypeKind::F32,  { ASTTypeKind::F64                                                                         } },
    };

    void
    SemanticAnalyzer::AnalyzeModule(Module *mod, bool isRoot) {
        Module *oldMod = _curMod;
        _curMod = mod;

        if (!isRoot) {
            ASTType selfType = ASTType(ASTTypeKind::Mod, mod->Name, false, 0);
            selfType.SetModule(mod);
            ASTVal selfVal = ASTVal(selfType, ASTValData { .i32Val = 0 }, false, false);
            selfVal.SetModule(mod);
            mod->Variables.emplace("self", Variable { .Name = "self", .Type = selfType, .Val = selfVal, .IsConst = true, .Access = AccessPriv });

            ASTType parentType = ASTType(ASTTypeKind::Mod, mod->Parent->Name, false, 0);
            parentType.SetModule(mod->Parent);
            ASTVal parentVal = ASTVal(parentType, ASTValData { .i32Val = 0 }, false, false);
            parentVal.SetModule(mod->Parent);
            mod->Variables.emplace("parent", Variable { .Name = "parent", .Type = parentType, .Val = parentVal, .IsConst = true, .Access = AccessPriv });
        }

        DeclareInModule(mod);

        for (auto *stmt : mod->AST) {
            Visit(stmt);
        }

        _curMod = oldMod;
    }

    void
    SemanticAnalyzer::DeclareInModule(Module *mod) {
        for (auto *stmt : mod->AST) {
            switch (stmt->GetKind()) {
                case NkVarDeclStmt: {
                    auto *vds = llvm::dyn_cast<VarDeclStmt>(stmt);
                    if (auto v = mod->FindGlobalVar(vds->GetName()); v && v->Parent == mod) {
                        _diag.Report(vds->GetStartLoc(), ErrRedefinitionVar)
                            << llvm::SMRange(vds->GetStartLoc(), vds->GetEndLoc());
                        continue;
                    }
                    resolveType(vds->GetType(), vds->GetStartLoc(), vds->GetEndLoc());
                    if (vds->GetType().GetTypeKind() == ASTTypeKind::Noth && !vds->GetType().IsPointer()) {
                        _diag.Report(vds->GetStartLoc(), ErrNothVar)
                            << llvm::SMRange(vds->GetStartLoc(), vds->GetEndLoc())
                            << vds->GetName();
                    }
                    else {
                        std::optional<ASTVal> val = vds->GetExpr() != nullptr ? Visit(vds->GetExpr()) : ASTVal::GetDefaultByType(vds->GetType());
                        if (vds->GetType().GetTypeKind() == ASTTypeKind::Struct && vds->GetExpr() == nullptr) {
                            Struct *s = vds->GetType().GetModule()->FindStruct(vds->GetType().GetVal());
                            val = ASTVal(ASTType(ASTTypeKind::Struct, s->Name, vds->IsConst(), 0, vds->GetType().GetModule(), vds->GetType().GetFullPath()),
                                         ASTValData { .i32Val = 0 }, false, false);
                        }
                        Variable var { .Name = vds->GetName(), .Type = vds->GetType(), .Val = val, .IsConst = vds->IsConst(), .Access = vds->GetAccess() };
                        if (vds->GetExpr()) {
                            implicitlyCast(var.Val.value(), var.Type, vds->GetExpr()->GetStartLoc(), vds->GetExpr()->GetEndLoc());
                        }
                        mod->Variables.emplace(vds->GetName(), var);
                    }
                    break;
                }
                case NkFunDeclStmt: {
                    auto *fds = llvm::dyn_cast<FunDeclStmt>(stmt);
                    if (fds->IsDeclaration()) {
                        _diag.Report(fds->GetStartLoc(), ErrCannotDeclareHere)
                            << llvm::SMRange(fds->GetStartLoc(), fds->GetEndLoc());
                        continue;
                    }
                    if (auto f = mod->FindFunction(fds->GetName()); f && f->Parent == mod) {
                        _diag.Report(fds->GetStartLoc(), ErrRedefinitionFun)
                            << llvm::SMRange(fds->GetStartLoc(), fds->GetEndLoc())
                            << fds->GetName();
                        continue;
                    }
                    for (auto &arg : fds->GetArgs()) {
                        resolveType(arg.GetType(), fds->GetStartLoc(), fds->GetEndLoc());
                    }
                    if (fds->GetName() == "main") {
                        auto args = fds->GetArgs();
                        if (args.size() != 0 && args.size() != 2) {
                            _diag.Report(fds->GetStartLoc(), ErrWrongMainSignature)
                                << llvm::SMRange(fds->GetStartLoc(), fds->GetEndLoc());
                        }
                        else if (args.size() == 2) {
                            if (!(args[0].GetType().GetTypeKind() == ASTTypeKind::I32 && !args[0].GetType().IsPointer() &&
                                  args[1].GetType().GetTypeKind() == ASTTypeKind::Char && args[1].GetType().GetPointerDepth() == 2)) {
                                _diag.Report(fds->GetStartLoc(), ErrWrongMainSignature)
                                    << llvm::SMRange(fds->GetStartLoc(), fds->GetEndLoc());
                            }
                        }
                    }
                    Function fun { .Name = fds->GetName(), .RetType = fds->GetRetType(), .Args = fds->GetArgs(), .Body = fds->GetBody(),
                                   .IsDeclaration = fds->IsDeclaration(), .Access = fds->GetAccess() };
                    resolveType(fun.RetType, fds->GetStartLoc(), fds->GetEndLoc());
                    mod->Functions.emplace(fun.Name, fun);
                    break;
                }
                case NkStructStmt: {
                    auto *ss = llvm::dyn_cast<StructStmt>(stmt);
                    if (auto s = mod->FindStruct(ss->GetName()); s && s->Parent == mod) {
                        _diag.Report(ss->GetStartLoc(), ErrRedefinitionStruct)
                            << llvm::SMRange(ss->GetStartLoc(), ss->GetEndLoc())
                            << ss->GetName();
                        continue;
                    }
                    mod->Structures.emplace(ss->GetName(), Struct { .Name = ss->GetName(), .Fields = {}, .Methods = {}, .TraitsImplements = {}, .Access = ss->GetAccess() });
                    Struct &s = mod->Structures.at(ss->GetName());
                    for (int i = 0; i < ss->GetBody().size(); ++i) {
                        if (ss->GetBody()[i]->GetKind() != NkVarDeclStmt) {
                            _diag.Report(ss->GetStartLoc(), ErrCannotBeHere)
                                << llvm::SMRange(ss->GetStartLoc(), ss->GetEndLoc());
                            continue;
                        }

                        VarDeclStmt *vds = llvm::dyn_cast<VarDeclStmt>(ss->GetBody()[i]);
                        resolveType(vds->GetType(), vds->GetStartLoc(), vds->GetEndLoc());
                        if (!vds->GetType().IsPointer() && vds->GetType().GetVal() == ss->GetName()) {
                            _diag.Report(vds->GetStartLoc(), ErrIncompleteType)
                                << llvm::SMRange(vds->GetStartLoc(), vds->GetEndLoc())
                                << vds->GetType().GetVal();
                        }
                        if (s.Fields.find(vds->GetName()) != s.Fields.end()) {
                            _diag.Report(vds->GetStartLoc(), ErrRedefinitionField)
                                << llvm::SMRange(vds->GetStartLoc(), vds->GetEndLoc())
                                << vds->GetName();
                            continue;
                        }
                        std::optional<ASTVal> val = vds->GetExpr() != nullptr ? Visit(vds->GetExpr()) : ASTVal::GetDefaultByType(vds->GetType());
                        if (vds->GetExpr()) {
                            implicitlyCast(val.value(), vds->GetType(), vds->GetExpr()->GetStartLoc(), vds->GetExpr()->GetEndLoc());
                        }
                        s.Fields.emplace(vds->GetName(), Field { .Name = vds->GetName(), .Val = val, .Type = vds->GetType(), .IsConst = vds->IsConst(),
                                         .Access = vds->GetAccess(), .ManualInitialized = false });
                    }
                    break;
                }
                case NkImplStmt: {
                    auto *is = llvm::dyn_cast<ImplStmt>(stmt);
                    resolveType(is->GetStructType(), is->GetStartLoc(), is->GetEndLoc());
                    if (!is->GetStructType().GetModule()) {
                        continue; // error will be handled in VisitImplStmt
                    }
                    Struct *s = is->GetStructType().GetModule()->FindStruct(is->GetStructType().GetVal());
                    if (!s) {
                        continue; // error will be handled in VisitImplStmt
                    }
                    bool isTraitImpl = is->GetTraitType() != ASTType();
                    const Trait *traitDef = nullptr;
                    std::unordered_map<std::string, bool> implementedTraitMethods;
                    
                    if (isTraitImpl) {
                        resolveType(is->GetTraitType(), is->GetStartLoc(), is->GetEndLoc());
                        if (!is->GetTraitType().GetModule()) {
                            continue;
                        }
                        Trait *t = is->GetTraitType().GetModule()->FindTrait(is->GetTraitType().GetVal());
                        if (!t) {
                            _diag.Report(is->GetStartLoc(), ErrUndeclaredTrait)
                                << llvm::SMRange(is->GetStartLoc(), is->GetEndLoc())
                                << is->GetTraitType().GetVal();
                            continue;
                        }
                        if (s->TraitsImplements.find(t->Name) != s->TraitsImplements.end()) {
                            _diag.Report(is->GetStartLoc(), ErrMultipleTraitImpl)
                                << llvm::SMRange(is->GetStartLoc(), is->GetEndLoc())
                                << t->Name
                                << s->Name;
                            continue;
                        }
                        traitDef = t;
                        
                        for (auto &method : traitDef->Methods) {
                            implementedTraitMethods[method.first] = !method.second.Fun.IsDeclaration;
                        }
                    }

                    std::vector<FunDeclStmt *> methods;
                    for (auto &stmt : is->GetBody()) {
                        if (stmt->GetKind() != NkFunDeclStmt) {
                            _diag.Report(stmt->GetStartLoc(), ErrCannotBeHere)
                                << llvm::SMRange(stmt->GetStartLoc(), stmt->GetEndLoc());
                            continue;
                        }
                        isConstMethod = true;
                        FunDeclStmt *method = llvm::dyn_cast<FunDeclStmt>(stmt);
                        if (method->IsDeclaration()) {
                            _diag.Report(method->GetStartLoc(), ErrCannotDeclareHere)
                                << llvm::SMRange(method->GetStartLoc(), method->GetEndLoc());
                            continue;
                        }
                        if (s->Methods.find(method->GetName()) != s->Methods.end()) {
                            _diag.Report(stmt->GetStartLoc(), ErrRedefinitionMethod)
                                << llvm::SMRange(stmt->GetStartLoc(), stmt->GetEndLoc())
                                << method->GetName();
                            continue;
                        }
                        resolveType(method->GetRetType(), method->GetStartLoc(), method->GetEndLoc());
                        for (auto &arg : method->GetArgs()) {
                            resolveType(arg.GetType(), method->GetStartLoc(), method->GetEndLoc());
                        }
                        if (isTraitImpl) {
                            auto tMethodIt = traitDef->Methods.find(method->GetName());
                            if (tMethodIt == traitDef->Methods.end()) {
                                _diag.Report(method->GetStartLoc(), ErrMethodNotInTrait)
                                    << llvm::SMRange(method->GetStartLoc(), method->GetEndLoc())
                                    << method->GetName()
                                    << traitDef->Name;
                                continue;
                            }

                            Function &traitFun = const_cast<Function &>(tMethodIt->second.Fun);
                            if (method->GetRetType() != traitFun.RetType) {
                                _diag.Report(method->GetStartLoc(), ErrCannotImplTraitMethod_RetTypeMismatch)
                                    << llvm::SMRange(method->GetStartLoc(), method->GetEndLoc())
                                    << method->GetName()
                                    << traitDef->Name
                                    << traitFun.RetType.ToString()
                                    << method->GetRetType().ToString();
                            }
                            if (method->GetArgs().size() != traitFun.Args.size()) {
                                 _diag.Report(method->GetStartLoc(), ErrCannotImplTraitMethod_FewArgs)
                                    << llvm::SMRange(method->GetStartLoc(), method->GetEndLoc())
                                    << method->GetName()
                                    << traitDef->Name
                                    << traitFun.Args.size()
                                    << method->GetArgs().size();
                            }
                            else {
                                for (int i = 0; i < method->GetArgs().size(); ++i) {
                                    if (method->GetArgs()[i].GetType() != traitFun.Args[i].GetType()) {
                                        _diag.Report(method->GetStartLoc(), ErrCannotImplTraitMethod_ArgTypeMismatch)
                                            << llvm::SMRange(method->GetStartLoc(), method->GetEndLoc())
                                            << method->GetName()
                                            << traitDef->Name
                                            << method->GetArgs()[i].GetName()
                                            << traitFun.Args[i].GetType().ToString()
                                            << method->GetArgs()[i].GetType().ToString(); 
                                    }
                                }
                            }

                            implementedTraitMethods[method->GetName()] = true;
                        }
                        methods.push_back(method);
                        Function fun { .Name = method->GetName(), .RetType = method->GetRetType(), .Args = method->GetArgs(), .Body = method->GetBody(),
                                       .IsDeclaration = method->IsDeclaration(), .Access = method->GetAccess() };
                        s->Methods.emplace(method->GetName(), Method { .Fun = fun, .IsConst = isConstMethod, .Access = method->GetAccess() });
                    }

                    if (isTraitImpl) {
                        for (auto const &[name, implemented] : implementedTraitMethods) {
                            if (!implemented) {
                                _diag.Report(is->GetStartLoc(), ErrNotImplTraitMethod)
                                    << llvm::SMRange(is->GetStartLoc(), is->GetEndLoc())
                                    << name
                                    << traitDef->Name;
                            }
                        }
                        s->TraitsImplements.emplace(traitDef->Name, *traitDef);
                    }

                    break;
                }
                case NkTraitDeclStmt: {
                    auto *tds = llvm::dyn_cast<TraitDeclStmt>(stmt);
                    if (auto t = mod->FindTrait(tds->GetName()); t && t->Parent == mod) {
                        _diag.Report(tds->GetStartLoc(), ErrRedefinitionTrait)
                            << llvm::SMRange(tds->GetStartLoc(), tds->GetEndLoc())
                            << tds->GetName();
                    }
                    Trait t { .Name = tds->GetName(), .Access = tds->GetAccess() };
                    for (auto stmt : tds->GetBody()) {
                        if (FunDeclStmt *method = llvm::dyn_cast<FunDeclStmt>(stmt)) {
                            if (!method->IsDeclaration()) {
                                _diag.Report(method->GetStartLoc(), ErrExpectedDeclarationInTrait)
                                    << llvm::SMRange(method->GetStartLoc(), method->GetEndLoc())
                                    << method->GetName()
                                    << tds->GetName();
                            }
                            resolveType(method->GetRetType(), tds->GetStartLoc(), tds->GetEndLoc());
                            for (auto &arg : method->GetArgs()) {
                                resolveType(arg.GetType(), tds->GetStartLoc(), tds->GetEndLoc());
                            }
                            Function fun { .Name = method->GetName(), .RetType = method->GetRetType(), .Args = method->GetArgs(), .Body = method->GetBody(),
                                           .IsDeclaration = method->IsDeclaration(), .Access = method->GetAccess() };
                            t.Methods.emplace(method->GetName(), Method { .Fun = fun, .Access = method->GetAccess() });
                        }
                        else {
                            _diag.Report(stmt->GetStartLoc(), ErrCannotBeHere)
                                << llvm::SMRange(stmt->GetStartLoc(), stmt->GetEndLoc());
                        }
                    }
                    mod->Traits.emplace(t.Name, t);
                    break;
                }
                case NkImportStmt: {
                    auto *is = llvm::dyn_cast<ImportStmt>(stmt);
                    std::string path;
                    if (is->IsLocalImport()) {
                        path = _parentDir + "/" + is->GetPath();
                    }
                    else {
                        path = ModuleManager::LibsPath + is->GetPath();
                    }
                    Module *import = ModuleManager::LoadModule(path, _srcMgr, _diag);
                    if (!import) {
                        _diag.Report(is->GetStartLoc(), ErrCouldNotFindMod)
                            << llvm::SMRange(is->GetStartLoc(), is->GetEndLoc())
                            << path + ".mr";
                        continue;
                    }
                    std::vector<std::string> parts = splitString(is->IsLocalImport() ? is->GetPath() : path, '/');
                    int i = is->IsLocalImport() ? 0 : 1;
                    Module *cur = mod;
                    for (; i < parts.size() - 1; ++i) {
                        std::string &name = parts[i];
                        Module *inner = nullptr;
                        if (auto it = cur->Submodules.find(name); it != cur->Submodules.end()) {
                            inner = it->second;
                        }
                        else if (auto it = cur->Imports.find(name); it != cur->Imports.end()) {
                            inner = it->second;
                        }
                        else {
                            inner = new Module(name, AccessPub);
                            if (cur == mod) {
                                cur->Imports.emplace(name, inner);
                            }
                            else {
                                cur->Submodules.emplace(name, inner);
                            }
                            inner->Parent = cur;
                        }
                        cur = inner;
                    }
                    if (cur == mod) {
                        cur->Imports.emplace(import->Name, import);
                    }
                    else {
                        cur->Submodules.emplace(import->Name, import);
                    }
                    import->Parent = cur;
                    AnalyzeModule(import);
                    break;
                }
                case NkModDeclStmt: {
                    auto *mds = llvm::dyn_cast<ModDeclStmt>(stmt);
                    if (mod->Submodules.find(mds->GetName()) != mod->Submodules.end()) {
                        _diag.Report(mds->GetStartLoc(), ErrRedefinitionMod)
                            << llvm::SMRange(mds->GetStartLoc(), mds->GetEndLoc())
                            << mds->GetName();
                        continue;
                    }
                    Module *sub = new Module(mds->GetName(), mds->GetAccess());
                    sub->AST = mds->GetBody();
                    mod->Submodules.emplace(mds->GetName(), sub);
                    sub->Parent = mod;
                    AnalyzeModule(sub);
                    break;
                }
                default:
                    break;
            }
        }
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitVarDeclStmt(VarDeclStmt *vds) {
        if (vds->GetAccess() == AccessPub && _vars.size() != 1) {
            _diag.Report(vds->GetStartLoc(), ErrCannotHaveAccessBeHere)
                << llvm::SMRange(vds->GetStartLoc(), vds->GetEndLoc());
        }
        if (_vars.top().find(vds->GetName()) != _vars.top().end()) {
            _diag.Report(vds->GetStartLoc(), ErrRedefinitionVar)
                << llvm::SMRange(vds->GetStartLoc(), vds->GetEndLoc())
                << vds->GetName();
        }
        resolveType(vds->GetType(), vds->GetStartLoc(), vds->GetEndLoc());
        if (vds->GetType().GetTypeKind() == ASTTypeKind::Noth && !vds->GetType().IsPointer()) {
            _diag.Report(vds->GetStartLoc(), ErrNothVar)
                << llvm::SMRange(vds->GetStartLoc(), vds->GetEndLoc())
                << vds->GetName();
        }
        else {
            std::optional<ASTVal> val = vds->GetExpr() != nullptr ? Visit(vds->GetExpr()) : ASTVal::GetDefaultByType(vds->GetType());
            if (vds->GetType().GetTypeKind() == ASTTypeKind::Struct && vds->GetExpr() == nullptr) {
                Struct *s = vds->GetType().GetModule()->FindStruct(vds->GetType().GetVal());
                if (!s) {
                    return std::nullopt;
                }
                val = ASTVal(ASTType(ASTTypeKind::Struct, s->Name, vds->IsConst(), 0, vds->GetType().GetModule(), vds->GetType().GetFullPath()),
                             ASTValData { .i32Val = 0 }, false, false);
            }
            Variable var { .Name = vds->GetName(), .Type = vds->GetType(), .Val = val, .IsConst = vds->IsConst(), .Access = vds->GetAccess() };
            if (vds->GetExpr()) {
                implicitlyCast(var.Val.value(), var.Type, vds->GetExpr()->GetStartLoc(), vds->GetExpr()->GetEndLoc());
            }
            _vars.top().emplace(vds->GetName(), var);
        }
        return std::nullopt;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitVarAsgnStmt(VarAsgnStmt *vas) {
        if (_vars.size() == 1) {
            _diag.Report(vas->GetStartLoc(), ErrCannotBeHere)
                << llvm::SMRange(vas->GetStartLoc(), vas->GetEndLoc());
        }
        if (vas->GetAccess() == AccessPub) {
            _diag.Report(vas->GetStartLoc(), ErrCannotHaveAccessBeHere)
                << llvm::SMRange(vas->GetStartLoc(), vas->GetEndLoc());
        }
        auto varsCopy = _vars;
        while (!varsCopy.empty()) {
            if (auto var = varsCopy.top().find(vas->GetName()); var != varsCopy.top().end()) {
                if (var->second.IsConst) {
                    _diag.Report(vas->GetStartLoc(), ErrAssignmentConst)
                        << llvm::SMRange(vas->GetStartLoc(), vas->GetEndLoc());
                    return std::nullopt;
                }
                ASTVal val = Visit(vas->GetExpr()).value_or(ASTVal::GetDefaultByType(ASTType::GetNothType()));
                ASTType type = var->second.Type;
                if (var->second.Type.IsPointer()) {
                    for (unsigned char dd = vas->GetDerefDepth(); dd > 0; type.Deref(), --dd) {
                        if (!type.IsPointer()) {
                            _diag.Report(vas->GetStartLoc(), ErrDerefFromNonPtr)
                                << llvm::SMRange(vas->GetStartLoc(),
                                                 llvm::SMLoc::getFromPointer(vas->GetStartLoc().getPointer() + vas->GetDerefDepth() + vas->GetName().length()));
                            break;
                        }
                        if (val.IsNil()) {
                            _diag.Report(vas->GetExpr()->GetStartLoc(), ErrDerefFromNil)
                                << llvm::SMRange(vas->GetStartLoc(), vas->GetEndLoc());
                            break;
                        }
                    }
                }
                val = implicitlyCast(val, type, vas->GetExpr()->GetStartLoc(), vas->GetExpr()->GetEndLoc());
                var->second.Val = val;
                return std::nullopt;
            }
            varsCopy.pop();
        }
        _diag.Report(vas->GetStartLoc(), ErrUndeclaredVariable)
            << getRange(vas->GetStartLoc(), vas->GetName().size())
            << vas->GetName();
        return std::nullopt;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitFunDeclStmt(FunDeclStmt *fds) {
        if (_vars.size() != 1) {
            _diag.Report(fds->GetStartLoc(), ErrCannotBeHere)
                << llvm::SMRange(fds->GetStartLoc(), fds->GetEndLoc());
        }
        if (fds->GetAccess() == AccessPub && _vars.size() != 1) {
            _diag.Report(fds->GetStartLoc(), ErrCannotHaveAccessBeHere)
                << llvm::SMRange(fds->GetStartLoc(), fds->GetEndLoc());
        }
        _vars.push({});
        for (auto &arg : fds->GetArgs()) {
            if (_vars.top().find(arg.GetName()) != _vars.top().end()) {
                _diag.Report(fds->GetStartLoc(), ErrRedefinitionVar)
                    << llvm::SMRange(fds->GetStartLoc(), fds->GetEndLoc())
                    << arg.GetName();
            }
            resolveType(arg.GetType(), fds->GetStartLoc(), fds->GetEndLoc());
            _vars.top().emplace(arg.GetName(), Variable { .Name = arg.GetName(), .Type = arg.GetType(),
                                                          .Val = arg.GetType().IsPointer() ? ASTVal(arg.GetType(), ASTValData { .i32Val = 0 }, false, false)
                                                                                           : ASTVal::GetDefaultByType(arg.GetType()),
                                                          .IsConst = arg.GetType().IsConst(), .Access = AccessPriv });
        }
        _funRetsTypes.push(fds->GetRetType());
        bool hasRet = false;
        for (auto stmt : fds->GetBody()) {
            if (stmt->GetKind() == NkRetStmt) {
                hasRet = true;
            }
            Visit(stmt);
        }
        _funRetsTypes.pop();
        _vars.pop();

        if (!hasRet && fds->GetRetType().GetTypeKind() != ASTTypeKind::Noth) {
            _diag.Report(fds->GetStartLoc(), ErrNotAllPathsReturnsValue)
                << llvm::SMRange(fds->GetStartLoc(), fds->GetEndLoc());
        }
        return std::nullopt;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitFunCallStmt(FunCallStmt *fcs) {
        if (fcs->GetAccess() == AccessPub) {
            _diag.Report(fcs->GetStartLoc(), ErrCannotHaveAccessBeHere)
                << llvm::SMRange(fcs->GetStartLoc(), fcs->GetEndLoc());
        }
        FunCallExpr *expr = new FunCallExpr(fcs->GetName(), fcs->GetArgs(), fcs->GetStartLoc(), fcs->GetEndLoc());
        VisitFunCallExpr(expr);
        delete expr;
        return std::nullopt;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitRetStmt(RetStmt *rs) {
        if (rs->GetAccess() == AccessPub) {
            _diag.Report(rs->GetStartLoc(), ErrCannotHaveAccessBeHere)
                << llvm::SMRange(rs->GetStartLoc(), rs->GetEndLoc());
        }
        ASTVal val = ASTVal::GetDefaultByType(ASTType::GetNothType());
        if (rs->GetExpr()) {
            val = Visit(rs->GetExpr()).value_or(ASTVal::GetDefaultByType(ASTType::GetNothType()));
        }
        implicitlyCast(val, _funRetsTypes.top(), rs->GetStartLoc(), rs->GetEndLoc());
        return std::nullopt;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitIfElseStmt(IfElseStmt *ies) {
        if (_vars.size() == 1) {
            _diag.Report(ies->GetStartLoc(), ErrCannotBeHere)
                << llvm::SMRange(ies->GetStartLoc(), ies->GetEndLoc());
        }
        if (ies->GetAccess() == AccessPub) {
            _diag.Report(ies->GetStartLoc(), ErrCannotHaveAccessBeHere)
                << llvm::SMRange(ies->GetStartLoc(), ies->GetEndLoc());
        }
        if (_funRetsTypes.empty()) {
            _diag.Report(ies->GetStartLoc(), ErrCannotBeHere)
                << llvm::SMRange(ies->GetStartLoc(), ies->GetEndLoc());
        }
        std::optional<ASTVal> cond = Visit(ies->GetCondition());
        implicitlyCast(cond.value(), ASTType(ASTTypeKind::Bool, "bool", false, 0), ies->GetCondition()->GetStartLoc(), ies->GetCondition()->GetEndLoc());
        _vars.push({});
        for (auto &stmt : ies->GetThenBody()) {
            Visit(stmt);
        }
        _vars.pop();
        _vars.push({});
        for (auto &stmt : ies->GetElseBody()) {
            Visit(stmt);
        }
        _vars.pop();
        return std::nullopt;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitForLoopStmt(ForLoopStmt *fls) {
        if (_vars.size() == 1) {
            _diag.Report(fls->GetStartLoc(), ErrCannotBeHere)
                << llvm::SMRange(fls->GetStartLoc(), fls->GetEndLoc());
        }
        if (fls->GetAccess() == AccessPub) {
            _diag.Report(fls->GetStartLoc(), ErrCannotHaveAccessBeHere)
                << llvm::SMRange(fls->GetStartLoc(), fls->GetEndLoc());
        }
        if (_funRetsTypes.empty()) {
            _diag.Report(fls->GetStartLoc(), ErrCannotBeHere)
                << llvm::SMRange(fls->GetStartLoc(), fls->GetEndLoc());
        }
        _vars.push({});
        ++_loopDeth;
        if (fls->GetIndexator()) {
            Visit(fls->GetIndexator());
        }
        if (fls->GetIteration()) {
            Visit(fls->GetIteration());
        }
        std::optional<ASTVal> cond = Visit(fls->GetCondition());
        implicitlyCast(cond.value(), ASTType(ASTTypeKind::Bool, "bool", false, 0), fls->GetCondition()->GetStartLoc(), fls->GetCondition()->GetEndLoc());
        for (auto &stmt : fls->GetBody()) {
            Visit(stmt);
        }
        --_loopDeth;
        _vars.pop();
        return std::nullopt;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitBreakStmt(BreakStmt *bs) {
        if (bs->GetAccess() == AccessPub) {
            _diag.Report(bs->GetStartLoc(), ErrCannotHaveAccessBeHere)
                << llvm::SMRange(bs->GetStartLoc(), bs->GetEndLoc());
        }
        if (_loopDeth == 0) {
            _diag.Report(bs->GetStartLoc(), ErrCannotBeHere)
                << llvm::SMRange(bs->GetStartLoc(), bs->GetEndLoc());
        }
        return std::nullopt;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitContinueStmt(ContinueStmt *cs) {
        if (cs->GetAccess() == AccessPub) {
            _diag.Report(cs->GetStartLoc(), ErrCannotHaveAccessBeHere)
                << llvm::SMRange(cs->GetStartLoc(), cs->GetEndLoc());
        }
        if (_loopDeth == 0) {
            _diag.Report(cs->GetStartLoc(), ErrCannotBeHere)
                << llvm::SMRange(cs->GetStartLoc(), cs->GetEndLoc());
        }
        return std::nullopt;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitStructStmt(StructStmt *ss) {
        if (_vars.size() != 1) {
            _diag.Report(ss->GetStartLoc(), ErrCannotBeHere)
                << llvm::SMRange(ss->GetStartLoc(), ss->GetEndLoc());
        }
        return std::nullopt;
    }
    
    std::optional<ASTVal>
    SemanticAnalyzer::VisitFieldAsgnStmt(FieldAsgnStmt *fas) {
        if (_vars.size() == 1) {
            _diag.Report(fas->GetStartLoc(), ErrCannotBeHere)
                << llvm::SMRange(fas->GetStartLoc(), fas->GetEndLoc());
        }
        std::optional<ASTVal> obj = Visit(fas->GetObject());
        resolveType(obj->GetType(), fas->GetStartLoc(), fas->GetEndLoc());
        if (obj->GetType().GetTypeKind() != ASTTypeKind::Struct &&
            obj->GetType().GetTypeKind() != ASTTypeKind::Mod) {
            _diag.Report(fas->GetStartLoc(), ErrAccessFromNonStruct)
                << llvm::SMRange(fas->GetStartLoc(), fas->GetEndLoc());
        }
        else if (obj->GetType().GetTypeKind() == ASTTypeKind::Struct) {
            fas->SetObjType(obj->GetType());
            bool objIsThis = false;
            if (fas->GetObject()->GetKind() == NkVarExpr) {
                VarExpr *ve = llvm::cast<VarExpr>(fas->GetObject());
                if (ve->GetName() == "this") {
                    objIsThis = true;
                    isConstMethod = false;
                }
            }
            Struct *s = obj->GetType().GetModule()->FindStruct(obj->GetType().GetVal());
            auto field = s->Fields.find(fas->GetName());
            if (field == s->Fields.end()) {
                _diag.Report(fas->GetStartLoc(), ErrUndeclaredField)
                    << llvm::SMRange(fas->GetStartLoc(), fas->GetEndLoc())
                    << fas->GetName()
                    << s->Name;
            }
            else {
                if (field->second.Access == AccessPriv && !objIsThis) {
                    _diag.Report(fas->GetStartLoc(), ErrFieldIsPrivate)
                        << llvm::SMRange(fas->GetStartLoc(), fas->GetEndLoc())
                        << fas->GetName();
                }
                if (field->second.IsConst || obj->GetType().IsConst()) {
                    _diag.Report(fas->GetStartLoc(), ErrAssignmentConst)
                        << llvm::SMRange(fas->GetStartLoc(), fas->GetEndLoc());
                    return std::nullopt;
                }
                implicitlyCast(Visit(fas->GetExpr()).value(), s->Fields.at(fas->GetName()).Type, fas->GetStartLoc(), fas->GetEndLoc());
            }
        }
        else {
            fas->SetObjType(obj->GetType());
            Module *mod = obj->GetModule();
            if (auto it = mod->Variables.find(fas->GetName()); it != mod->Variables.end()) {
                if (it->second.Access == AccessPriv && mod != _curMod) {
                    _diag.Report(fas->GetStartLoc(), ErrVarIsPrivate)
                        << llvm::SMRange(fas->GetStartLoc(), fas->GetEndLoc())
                        << fas->GetName();
                    return std::nullopt;
                }
                if (it->second.IsConst || obj->GetType().IsConst()) {
                    _diag.Report(fas->GetStartLoc(), ErrAssignmentConst)
                        << llvm::SMRange(fas->GetStartLoc(), fas->GetEndLoc());
                    return std::nullopt;
                }
                implicitlyCast(Visit(fas->GetExpr()).value(), it->second.Type, fas->GetStartLoc(), fas->GetEndLoc());
                return std::nullopt;
            }
            else {
                _diag.Report(fas->GetStartLoc(), ErrUndeclaredVariable)
                    << llvm::SMRange(fas->GetStartLoc(), fas->GetEndLoc())
                    << fas->GetName();
                return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
            }
        }
        return std::nullopt;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitImplStmt(ImplStmt *is) {
        if (_vars.size() != 1) {
            _diag.Report(is->GetStartLoc(), ErrCannotBeHere)
                << llvm::SMRange(is->GetStartLoc(), is->GetEndLoc());
        }
        resolveType(is->GetStructType(), is->GetStartLoc(), is->GetEndLoc());
        if (!is->GetStructType().GetModule()) {
            return std::nullopt;
        }
        Struct *s = is->GetStructType().GetModule()->FindStruct(is->GetStructType().GetVal());
        if (!s) {
            _diag.Report(is->GetStartLoc(), ErrUndeclaredStructure)
                 << llvm::SMRange(is->GetStartLoc(), is->GetEndLoc())
                 << is->GetStructType().GetVal();
            return std::nullopt;
        }

        std::vector<FunDeclStmt *> methods;
        for (auto &stmt : is->GetBody()) {
            isConstMethod = true;
            FunDeclStmt *method = llvm::dyn_cast<FunDeclStmt>(stmt);
            _vars.push({});
            ASTType thisType = ASTType(ASTTypeKind::Struct, s->Name, false, 0, is->GetStructType().GetModule());
            _vars.top().emplace("this", Variable { .Name = "this", .Type = thisType, .Val = ASTVal::GetDefaultByType(thisType), .IsConst = false, .Access = AccessPriv });
            for (auto &arg : method->GetArgs()) {
                if (_vars.top().find(arg.GetName()) != _vars.top().end()) {
                    _diag.Report(method->GetStartLoc(), ErrRedefinitionVar)
                        << llvm::SMRange(method->GetStartLoc(), method->GetEndLoc())
                        << arg.GetName();
                }
                resolveType(arg.GetType(), method->GetStartLoc(), method->GetEndLoc());
                _vars.top().emplace(arg.GetName(), Variable { .Name = arg.GetName(), .Type = arg.GetType(),
                                                              .Val = arg.GetType().IsPointer() ? ASTVal(arg.GetType(), ASTValData { .i32Val = 0 }, false, false)
                                                                                               : ASTVal::GetDefaultByType(arg.GetType()),
                                                              .IsConst = arg.GetType().IsConst(), .Access = AccessPriv });
            }
            _funRetsTypes.push(method->GetRetType());
            bool hasRet;
            for (auto stmt : method->GetBody()) {
                if (stmt->GetKind() == NkRetStmt) {
                    hasRet = true;
                }
                Visit(stmt);
            }
            _funRetsTypes.pop();
            _vars.pop();

            if (!hasRet && method->GetRetType().GetTypeKind() != ASTTypeKind::Noth) {
                _diag.Report(method->GetStartLoc(), ErrNotAllPathsReturnsValue)
                    << llvm::SMRange(method->GetStartLoc(), method->GetEndLoc());
            }
        }
        return std::nullopt;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitMethodCallStmt(MethodCallStmt *mcs) {
        if (mcs->GetAccess() == AccessPub) {
            _diag.Report(mcs->GetStartLoc(), ErrCannotHaveAccessBeHere)
                << llvm::SMRange(mcs->GetStartLoc(), mcs->GetEndLoc());
        }
        MethodCallExpr *expr = new MethodCallExpr(mcs->GetObject(), mcs->GetName(), mcs->GetArgs(), mcs->GetStartLoc(), mcs->GetEndLoc());
        std::optional<ASTVal> obj = Visit(mcs->GetObject());
        resolveType(obj->GetType(), mcs->GetStartLoc(), mcs->GetEndLoc());
        mcs->SetObjType(obj->GetType());
        expr->SetObjType(mcs->GetObjType());
        VisitMethodCallExpr(expr);
        delete expr;
        return std::nullopt;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitTraitDeclStmt(TraitDeclStmt *tds) {
        if (_vars.size() != 1) {
            _diag.Report(tds->GetStartLoc(), ErrCannotBeHere)
                << llvm::SMRange(tds->GetStartLoc(), tds->GetEndLoc());
        }
        if (_vars.size() != 1 && tds->GetAccess() == AccessPub) {
            _diag.Report(tds->GetStartLoc(), ErrCannotHaveAccessBeHere)
                << llvm::SMRange(tds->GetStartLoc(), tds->GetEndLoc());
        }
        return std::nullopt;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitEchoStmt(EchoStmt *es) {
        if (_vars.size() == 1) {
            _diag.Report(es->GetStartLoc(), ErrCannotBeHere)
                << llvm::SMRange(es->GetStartLoc(), es->GetEndLoc());
        }
        if (es->GetAccess() != AccessPriv) {
            _diag.Report(es->GetStartLoc(), ErrCannotHaveAccessBeHere)
                << llvm::SMRange(es->GetStartLoc(), es->GetEndLoc());
        }
        std::optional<ASTVal> val = Visit(es->GetExpr());
        es->SetExprType(resolveType(val->GetType(), es->GetStartLoc(), es->GetEndLoc()));
        if (val->GetType().GetTypeKind() == ASTTypeKind::Noth) {
            _diag.Report(es->GetStartLoc(), ErrEchoTypeIsNoth)
                << llvm::SMRange(es->GetStartLoc(), es->GetEndLoc());
        }
        return val;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitDelStmt(DelStmt *ds) {
        if (_vars.size() == 1) {
            _diag.Report(ds->GetStartLoc(), ErrCannotBeHere)
                << llvm::SMRange(ds->GetStartLoc(), ds->GetEndLoc());
        }
        std::optional<ASTVal> val = Visit(ds->GetExpr());
        resolveType(val->GetType(), ds->GetStartLoc(), ds->GetEndLoc());
        if (!val->GetType().IsPointer()) {
            _diag.Report(ds->GetStartLoc(), ErrDelOfNonPtr)
                << llvm::SMRange(ds->GetStartLoc(), ds->GetEndLoc());
            return std::nullopt;
        }
        if (val->IsNil()) {
            _diag.Report(ds->GetStartLoc(), ErrDelOfNil)
                << llvm::SMRange(ds->GetStartLoc(), ds->GetEndLoc());
            return std::nullopt;
        }
        if (!val->CreatedByNew()) {
            _diag.Report(ds->GetStartLoc(), ErrDelOfCreatedNotByNew)
                << llvm::SMRange(ds->GetStartLoc(), ds->GetEndLoc());
            return std::nullopt;
        }
        return std::nullopt;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitImportStmt(ImportStmt *is) {
        return std::nullopt;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitModDeclStmt(ModDeclStmt *mds) {
        return std::nullopt;
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitBinaryExpr(BinaryExpr *be) {
        checkBinaryExpr(be);
        std::optional<ASTVal> lhs = Visit(be->GetLHS());
        if (lhs == std::nullopt) {
            return lhs;
        }
        std::optional<ASTVal> rhs = Visit(be->GetRHS());
        if (rhs == std::nullopt) {
            return rhs;
        }
        resolveType(lhs->GetType(), be->GetStartLoc(), be->GetEndLoc());
        resolveType(rhs->GetType(), be->GetStartLoc(), be->GetEndLoc());
        be->SetLHSType(lhs->GetType());
        be->SetRHSType(rhs->GetType());
        if (lhs->GetType().IsPointer()) {
            if (llvm::dyn_cast<NilExpr>(be->GetRHS())) {
                return ASTVal::GetVal(lhs->IsNil(), ASTType(ASTTypeKind::Bool, "bool", false, 0));
            }
            if (rhs->GetType().IsPointer()) {
                return ASTVal::GetDefaultByType(ASTType(ASTTypeKind::I64, "i64", false, 0));
            }
            return lhs;
        }
        double lhsVal = lhs->AsDouble();
        double rhsVal = rhs->AsDouble();
        double res;
        bool returnBool = false;
        switch (be->GetOp().GetKind()) {
            #define EVAL(op) lhsVal op rhsVal
            case TkPlus:
                res = EVAL(+);
                break;
            case TkMinus:
                res = EVAL(+);
                break;
            case TkStar:
                res = EVAL(*);
                break;
            case TkSlash:
                res = EVAL(/);
                break;
            case TkPercent:
                res = std::fmod(lhsVal, rhsVal);
                break;
            case TkLogAnd:
                res = EVAL(&&);
                returnBool = true;
                break;
            case TkLogOr:
                res = EVAL(||);
                returnBool = true;
                break;
            case TkAnd:
                res = static_cast<long>(lhsVal) & static_cast<long>(rhsVal);
                break;
            case TkOr:
                res = static_cast<long>(lhsVal) | static_cast<long>(rhsVal);
                break;
            case TkGt:
                res = EVAL(>);
                returnBool = true;
                break;
            case TkGtEq:
                res = EVAL(>=);
                returnBool = true;
                break;
            case TkLt:
                res = EVAL(<);
                returnBool = true;
                break;
            case TkLtEq:
                res = EVAL(<=);
                returnBool = true;
                break;
            case TkEqEq:
                res = EVAL(==);
                returnBool = true;
                break;
            case TkNotEq:
                res = EVAL(!=);
                returnBool = true;
                break;
            default: {}
            #undef EVAL
        }
        if (returnBool) {
            return ASTVal::GetVal(res, ASTType(ASTTypeKind::Bool, "bool", false, 0));
        }
        return ASTVal::GetVal(res, ASTType::GetCommon(lhs->GetType(), rhs->GetType()));
    }
    
    std::optional<ASTVal>
    SemanticAnalyzer::VisitUnaryExpr(UnaryExpr *ue) {
        std::optional<ASTVal> rhs = Visit(ue->GetRHS());
        resolveType(rhs->GetType(), ue->GetStartLoc(), ue->GetEndLoc());
        if (rhs == std::nullopt) {
            return rhs;
        }
        double val = rhs->AsDouble();
        bool returnBool = false;
        switch (ue->GetOp().GetKind()) {
            case TkMinus:
                if (rhs->GetType().GetTypeKind() < ASTTypeKind::Char || rhs->GetType().GetTypeKind() > ASTTypeKind::F64) {
                    _diag.Report(ue->GetRHS()->GetStartLoc(), ErrTypeMismatchNotNum)
                        << llvm::SMRange(ue->GetRHS()->GetStartLoc(), ue->GetRHS()->GetEndLoc())
                        << rhs->GetType().ToString();
                }
                val *= -1;
                break;
            case TkBang:
                if (rhs->GetType().GetTypeKind() != ASTTypeKind::Bool) {
                    _diag.Report(ue->GetRHS()->GetStartLoc(), ErrTypeMismatchNotBool)
                        << llvm::SMRange(ue->GetRHS()->GetStartLoc(), ue->GetRHS()->GetEndLoc())
                        << rhs->GetType().ToString();
                }
                val = !val;
                returnBool = true;
                break;
            default: {}
        }
        return ASTVal::GetVal(val, rhs->GetType());
    }
    
    std::optional<ASTVal>
    SemanticAnalyzer::VisitVarExpr(VarExpr *ve) {
        auto varsCopy = _vars;
        while (!varsCopy.empty()) {
            if (auto var = varsCopy.top().find(ve->GetName()); var != varsCopy.top().end()) {
                ASTType type = ASTType(var->second.Type.GetTypeKind(), var->second.Type.GetVal(), var->second.IsConst, var->second.Type.GetPointerDepth(),
                                       var->second.Type.GetModule(), var->second.Type.GetFullPath());
                if (var->second.Type.GetTypeKind() == ASTTypeKind::Trait) {
                    return ASTVal(type, ASTValData { .i32Val = 0 }, var->second.Val->IsNil(), var->second.Val->CreatedByNew());
                }
                return ASTVal(type, var->second.Val->GetData(), var->second.Val->IsNil(), var->second.Val->CreatedByNew());
            }
            varsCopy.pop();
        }
        if (ve->GetName() == "self" || ve->GetName() == "parent") {
            return _curMod->Variables.at(ve->GetName()).Val;
        }
        if (auto mod = _curMod->FindModule(ve->GetName())) {
            auto val = ASTVal(ASTType(ASTTypeKind::Mod, ve->GetName(), false, 0), ASTValData { .i32Val = 0 }, false, false);
            val.GetType().SetModule(mod);
            val.SetModule(mod);
            return val;
        }
        _diag.Report(ve->GetStartLoc(), ErrUndeclaredVariable)
            << llvm::SMRange(ve->GetStartLoc(), ve->GetEndLoc())
            << ve->GetName();
        return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
    }
    
    std::optional<ASTVal>
    SemanticAnalyzer::VisitLiteralExpr(LiteralExpr *le) {
        return le->GetVal();
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitFunCallExpr(FunCallExpr *fce) {
        Function *fun = _curMod->FindFunction(fce->GetName());
        if (fun) {
            if (fun->Args.size() != fce->GetArgs().size()) {
                _diag.Report(fce->GetStartLoc(), ErrFewArgs)
                    << llvm::SMRange(fce->GetStartLoc(), fce->GetEndLoc())
                    << fce->GetName()
                    << fun->Args.size()
                    << fce->GetArgs().size();
                return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
            }
            for (int i = 0; i < fun->Args.size(); ++i) {
                resolveType(fun->Args[i].GetType(), fce->GetStartLoc(), fce->GetEndLoc());
                implicitlyCast(Visit(fce->GetArgs()[i]).value(), fun->Args[i].GetType(), fce->GetArgs()[i]->GetStartLoc(), fce->GetArgs()[i]->GetEndLoc());
            }
            if (fun->RetType.GetTypeKind() != ASTTypeKind::Noth) {
                return ASTVal::GetDefaultByType(fun->RetType);
            }
            return ASTVal::GetDefaultByType(ASTType::GetNothType());
        }
        _diag.Report(fce->GetStartLoc(), ErrUndeclaredFunction)
            << getRange(fce->GetStartLoc(), fce->GetName().size())
            << fce->GetName();
        return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitStructExpr(StructExpr *se) {
        resolveType(se->GetType(), se->GetStartLoc(), se->GetEndLoc());
        if (!se->GetType().GetModule()) {
            return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
        }
        Struct *s = se->GetType().GetModule()->FindStruct(se->GetType().GetVal());
        if (!s) {
            _diag.Report(se->GetStartLoc(), ErrUndeclaredStructure)
                << llvm::SMRange(se->GetStartLoc(), se->GetEndLoc())
                << se->GetType().GetVal();
            return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
        }
        for (int i = 0; i < se->GetInitializer().size(); ++i) {
            std::string name = se->GetInitializer()[i].first;
            if (s->Fields.find(name) != s->Fields.end()) {
                auto field = s->Fields.at(name);
                if (field.ManualInitialized) {
                    _diag.Report(se->GetStartLoc(), ErrFieldInitialized)
                        << llvm::SMRange(se->GetStartLoc(), se->GetEndLoc())
                        << name;
                }
                else {
                    field.Val = Visit(se->GetInitializer()[i].second);
                    field.ManualInitialized = true;
                }
            }
            else {
                _diag.Report(se->GetStartLoc(), ErrUndeclaredField)
                    << llvm::SMRange(se->GetStartLoc(), se->GetEndLoc())
                    << name
                    << s->Name;
            }
        }
        return ASTVal(ASTType(ASTTypeKind::Struct, s->Name, false, 0, se->GetType().GetModule()), ASTValData { .i32Val = 0 }, false, false);
    }


    std::optional<ASTVal>
    SemanticAnalyzer::VisitFieldAccessExpr(FieldAccessExpr *fae) {
        std::optional<ASTVal> obj = Visit(fae->GetObject());
        resolveType(obj->GetType(), fae->GetStartLoc(), fae->GetEndLoc());
        if (obj->GetType().GetTypeKind() != ASTTypeKind::Struct &&
            obj->GetType().GetTypeKind() != ASTTypeKind::Mod) {
            _diag.Report(fae->GetStartLoc(), ErrAccessFromNonStruct)
                << llvm::SMRange(fae->GetStartLoc(), fae->GetEndLoc());
        }
        else if (obj->GetType().GetTypeKind() == ASTTypeKind::Struct) {
            fae->SetObjType(obj->GetType());
            bool objIsThis = false;
            if (fae->GetObject()->GetKind() == NkVarExpr) {
                VarExpr *ve = llvm::cast<VarExpr>(fae->GetObject());
                if (ve->GetName() == "this") {
                    objIsThis = true;
                }
            }
            Struct *s = obj->GetType().GetModule()->FindStruct(obj->GetType().GetVal());
            auto field = s->Fields.find(fae->GetName());
            if (field == s->Fields.end()) {
                _diag.Report(fae->GetStartLoc(), ErrUndeclaredField)
                    << llvm::SMRange(fae->GetStartLoc(), fae->GetEndLoc())
                    << fae->GetName()
                    << s->Name;
            }
            else {
                if (field->second.Access == AccessPriv && !objIsThis) {
                    _diag.Report(fae->GetStartLoc(), ErrFieldIsPrivate)
                        << llvm::SMRange(fae->GetStartLoc(), fae->GetEndLoc())
                        << fae->GetName();
                }
                return s->Fields.at(fae->GetName()).Val;
            }
        }
        else {
            fae->SetObjType(obj->GetType());
            Module *mod = obj->GetModule();
            if (auto it = mod->Variables.find(fae->GetName()); it != mod->Variables.end()) {
                if (it->second.Access == AccessPriv && mod != _curMod) {
                    _diag.Report(fae->GetStartLoc(), ErrVarIsPrivate)
                        << llvm::SMRange(fae->GetStartLoc(), fae->GetEndLoc())
                        << fae->GetName();
                    return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
                }
                return it->second.Val;
            }
            else if (auto it = mod->Imports.find(fae->GetName()); it != mod->Imports.end()) {
                auto val = ASTVal(ASTType(ASTTypeKind::Mod, fae->GetName(), false, 0), ASTValData { .i32Val = 0 }, false, false);
                val.GetType().SetModule(it->second);
                val.SetModule(it->second);
                return val;
            }
            else if (auto it = mod->Submodules.find(fae->GetName()); it != mod->Submodules.end()) {
                if (it->second->Access == AccessPriv) {
                    _diag.Report(fae->GetStartLoc(), ErrModIsPrivate)
                        << llvm::SMRange(fae->GetStartLoc(), fae->GetEndLoc())
                        << fae->GetName();
                    return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
                }
                auto val = ASTVal(ASTType(ASTTypeKind::Mod, fae->GetName(), false, 0), ASTValData { .i32Val = 0 }, false, false);
                val.GetType().SetModule(it->second);
                val.SetModule(it->second);
                return val;
            }
            else {
                _diag.Report(fae->GetStartLoc(), ErrUndeclaredVariable)
                    << llvm::SMRange(fae->GetStartLoc(), fae->GetEndLoc())
                    << fae->GetName();
                return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
            }
        }
        return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitMethodCallExpr(MethodCallExpr *mce) {
        std::optional<ASTVal> obj = Visit(mce->GetObject());
        resolveType(obj->GetType(), mce->GetStartLoc(), mce->GetEndLoc());
        if (obj->GetType().GetTypeKind() != ASTTypeKind::Struct &&
            obj->GetType().GetTypeKind() != ASTTypeKind::Trait &&
            obj->GetType().GetTypeKind() != ASTTypeKind::Mod) {
            _diag.Report(mce->GetStartLoc(), ErrAccessFromNonStruct)
                << llvm::SMRange(mce->GetStartLoc(), mce->GetEndLoc());
        }
        else if (obj->GetType().GetTypeKind() != ASTTypeKind::Mod) {
            mce->SetObjType(obj->GetType());
            bool objIsThis = false;
            if (mce->GetObject()->GetKind() == NkVarExpr) {
                VarExpr *ve = llvm::cast<VarExpr>(mce->GetObject());
                if (ve->GetName() == "this") {
                    objIsThis = true;
                }
            }
            std::unordered_map<std::string, Method> *methods = nullptr;
            std::string contextName;
            if (obj->GetType().GetTypeKind() == ASTTypeKind::Struct) {
                Struct *s = obj->GetType().GetModule()->FindStruct(obj->GetType().GetVal());
                methods = &s->Methods;
                contextName = s->Name;
            }
            else {
                Trait *t = obj->GetType().GetModule()->FindTrait(obj->GetType().GetVal());
                methods = &t->Methods;
                contextName = t->Name;
            }
            auto method = methods->find(mce->GetName());
            if (method == methods->end()) {
                _diag.Report(mce->GetStartLoc(), ErrUndeclaredMethod)
                    << llvm::SMRange(mce->GetStartLoc(), mce->GetEndLoc())
                    << mce->GetName()
                    << contextName;
            }
            else {
                if (isConstMethod) {
                    isConstMethod = method->second.IsConst;
                }

                if (method->second.Access == AccessPriv && !objIsThis) {
                    _diag.Report(mce->GetStartLoc(), ErrMethodIsPrivate)
                        << llvm::SMRange(mce->GetStartLoc(), mce->GetEndLoc())
                        << mce->GetName();
                }
                if (obj->GetType().IsConst() && !isConstMethod) {
                    _diag.Report(mce->GetStartLoc(), ErrCallingNonConstMethodForConstObj)
                        << llvm::SMRange(mce->GetStartLoc(), mce->GetEndLoc())
                        << mce->GetName();
                }
                if (method->second.Fun.Args.size() != mce->GetArgs().size()) {
                    _diag.Report(mce->GetStartLoc(), ErrFewArgs)
                        << llvm::SMRange(mce->GetStartLoc(), mce->GetEndLoc())
                        << mce->GetName()
                        << method->second.Fun.Args.size()
                        << mce->GetArgs().size();
                    return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
                }
                for (int i = 0; i < method->second.Fun.Args.size(); ++i) {
                    resolveType(method->second.Fun.Args[i].GetType(), mce->GetStartLoc(), mce->GetEndLoc());
                    implicitlyCast(Visit(mce->GetArgs()[i]).value(), method->second.Fun.Args[i].GetType(), mce->GetArgs()[i]->GetStartLoc(), mce->GetArgs()[i]->GetEndLoc());
                }
                if (method->second.Fun.RetType.GetTypeKind() != ASTTypeKind::Noth) {
                    return ASTVal::GetDefaultByType(method->second.Fun.RetType);
                }
                return ASTVal::GetDefaultByType(ASTType::GetNothType());
            }
        }
        else {
            mce->SetObjType(obj->GetType());
            Module *mod = obj->GetModule();
            if (auto it = mod->Functions.find(mce->GetName()); it != mod->Functions.end()) {
                if (it->second.Access == AccessPriv && mod != _curMod) {
                    _diag.Report(mce->GetStartLoc(), ErrFunIsPrivate)
                        << llvm::SMRange(mce->GetStartLoc(), mce->GetEndLoc())
                        << mce->GetName();
                }
                if (it->second.Args.size() != mce->GetArgs().size()) {
                    _diag.Report(mce->GetStartLoc(), ErrFewArgs)
                        << llvm::SMRange(mce->GetStartLoc(), mce->GetEndLoc())
                        << mce->GetName()
                        << it->second.Args.size()
                        << mce->GetArgs().size();
                    return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
                }
                for (int i = 0; i < it->second.Args.size(); ++i) {
                    resolveType(it->second.Args[i].GetType(), mce->GetStartLoc(), mce->GetEndLoc());
                    implicitlyCast(Visit(mce->GetArgs()[i]).value(), it->second.Args[i].GetType(), mce->GetArgs()[i]->GetStartLoc(), mce->GetArgs()[i]->GetEndLoc());
                }
                if (it->second.RetType.GetTypeKind() != ASTTypeKind::Noth) {
                    return ASTVal::GetDefaultByType(it->second.RetType);
                }
                return ASTVal::GetDefaultByType(ASTType::GetNothType());
            }
            else {
                _diag.Report(mce->GetStartLoc(), ErrUndeclaredFunction)
                    << llvm::SMRange(mce->GetStartLoc(), mce->GetEndLoc())
                    << mce->GetName();
                return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
            }
        }
        return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitNilExpr(NilExpr *ne) {
        return ASTVal(ASTType(ASTTypeKind::Nil, "nil", false, 0), ASTValData { .i32Val = 0 }, true, false);
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitDerefExpr(DerefExpr *de) {
        std::optional<ASTVal> val = Visit(de->GetExpr());
        resolveType(val->GetType(), de->GetStartLoc(), de->GetEndLoc());
        if (!val->GetType().IsPointer()) {
            _diag.Report(de->GetExpr()->GetStartLoc(), ErrDerefFromNonPtr)
                << llvm::SMRange(de->GetStartLoc(), de->GetEndLoc());
            return val;
        }
        if (val->IsNil()) {
            _diag.Report(de->GetExpr()->GetStartLoc(), ErrDerefFromNil)
                << llvm::SMRange(de->GetStartLoc(), de->GetEndLoc());
            return val;
        }
        if (val->GetType().GetTypeKind() == ASTTypeKind::Noth && val->GetType().GetPointerDepth() == 1) {
            _diag.Report(de->GetExpr()->GetStartLoc(), ErrDerefNothPtr)
                << llvm::SMRange(de->GetStartLoc(), de->GetEndLoc());
            return val;
        }
        de->SetExprType(val->GetType());
        return ASTVal(val->GetType().Deref(), val->GetData(), false, val->CreatedByNew());
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitRefExpr(RefExpr *re) {
        std::optional<ASTVal> val = Visit(re->GetExpr());
        resolveType(val->GetType(), re->GetStartLoc(), re->GetEndLoc());
        if (re->GetExpr()->GetKind() == NkVarExpr) {
            return ASTVal(val->GetType().Ref(), val->GetData(), false, val->CreatedByNew());
        }
        if (FieldAccessExpr *fae = llvm::dyn_cast<FieldAccessExpr>(re->GetExpr())) {
            if (fae->GetObject()->GetKind() == NkVarExpr) {
                return ASTVal(val->GetType().Ref(), val->GetData(), false, val->CreatedByNew());
            }
        }
        _diag.Report(re->GetExpr()->GetStartLoc(), ErrRefFromRVal)
            << llvm::SMRange(re->GetStartLoc(), re->GetEndLoc());
        return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitNewExpr(NewExpr *ne) {
        if (ne->GetStructExpr()) {
            VisitStructExpr(ne->GetStructExpr());
        }
        resolveType(ne->GetType(), ne->GetStartLoc(), ne->GetEndLoc());
        if (ne->GetType().GetTypeKind() == ASTTypeKind::Trait) {
            _diag.Report(ne->GetStartLoc(), ErrNewOnTrait)
                << llvm::SMRange(ne->GetStartLoc(), ne->GetEndLoc());
            return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
        }
        if (ne->GetType().GetTypeKind() == ASTTypeKind::Noth) {
            _diag.Report(ne->GetStartLoc(), ErrNewOnNoth)
                << llvm::SMRange(ne->GetStartLoc(), ne->GetEndLoc());
            return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
        }
        return ASTVal(ne->GetType().Ref(), ASTValData { .i32Val = 0 }, false, true);
    }

    ASTType
    SemanticAnalyzer::resolveType(ASTType &type, llvm::SMLoc startLoc, llvm::SMLoc endLoc) {
        if (type.GetTypeKind() != ASTTypeKind::Unknown) {
            return type;
        }

        const std::string &typeName = type.GetVal();
        if (!type.GetFullPath().empty()) {
            auto path = splitString(type.GetFullPath(), '.');
            path.pop_back(); // last part is name of type
            Module *cur = _curMod;
            for (auto part : path) {
                Module *tmp;
                if (part == "self") {
                    tmp = _curMod;
                }
                else if (part == "parent") {
                    tmp = _curMod->Parent;
                }
                else {
                    tmp = cur->FindModule(part);
                }
                if (!tmp) {
                    _diag.Report(startLoc, ErrUndeclaredMod)
                        << llvm::SMRange(startLoc, endLoc)
                        << part
                        << cur->Name;
                    type = ASTType(ASTTypeKind::I32, "i32", false, 0);
                    break;
                }
                cur = tmp;
            }
            if (auto *s = cur->FindStruct(typeName)) {
                if (s->Access == AccessPriv && s->Parent != _curMod) {
                    _diag.Report(startLoc, ErrStructIsPrivate)
                        << llvm::SMRange(startLoc, endLoc)
                        << s->Name;
                }
                type.SetTypeKind(ASTTypeKind::Struct);
                type.SetModule(s->Parent);
            }
            else if (auto *t = cur->FindTrait(typeName)) {
                if (t->Access == AccessPriv && t->Parent != _curMod) {
                    _diag.Report(startLoc, ErrTraitIsPrivate)
                        << llvm::SMRange(startLoc, endLoc)
                        << t->Name;
                }
                type.SetTypeKind(ASTTypeKind::Trait);
                type.SetModule(t->Parent);
            }
        }
        else {
            if (auto *s = _curMod->FindStruct(typeName)) {
                if (s->Access == AccessPriv && s->Parent != _curMod) {
                    _diag.Report(startLoc, ErrStructIsPrivate)
                        << llvm::SMRange(startLoc, endLoc)
                        << s->Name;
                }
                type.SetTypeKind(ASTTypeKind::Struct);
                type.SetModule(s->Parent);
            }
            else if (auto *t = _curMod->FindTrait(typeName)) {
                if (t->Access == AccessPriv && t->Parent != _curMod) {
                    _diag.Report(startLoc, ErrTraitIsPrivate)
                        << llvm::SMRange(startLoc, endLoc)
                        << t->Name;
                }
                type.SetTypeKind(ASTTypeKind::Trait);
                type.SetModule(t->Parent);
            }
        }
        if (type.GetTypeKind() == ASTTypeKind::Unknown) {
            _diag.Report(startLoc, ErrUndeclaredType)
                << llvm::SMRange(startLoc, endLoc)
                << type.GetVal();
            type = ASTType(ASTTypeKind::I32, "i32", false, 0);
        }
        return type;
    }

    llvm::SMRange
    SemanticAnalyzer::getRange(llvm::SMLoc start, int len) const {
        return llvm::SMRange(start, llvm::SMLoc::getFromPointer(start.getPointer() + len));
    }

    void
    SemanticAnalyzer::checkBinaryExpr(BinaryExpr *be) {
        ASTVal lhs = Visit(be->GetLHS()).value();
        resolveType(lhs.GetType(), be->GetStartLoc(), be->GetEndLoc());
        ASTVal rhs = Visit(be->GetRHS()).value();
        resolveType(rhs.GetType(), be->GetStartLoc(), be->GetEndLoc());
        switch (be->GetOp().GetKind()) {
            case TkPlus:
            case TkMinus:
            case TkStar:
            case TkSlash:
            case TkPercent:
                if (lhs.GetType().IsPointer() && rhs.GetType().IsPointer() && be->GetOp().GetKind() == TkPlus) {
                    _diag.Report(be->GetStartLoc(), ErrUnsupportedTypeForOperator)
                        << llvm::SMRange(be->GetStartLoc(), be->GetEndLoc())
                        << be->GetOp().GetText()
                        << lhs.GetType().ToString()
                        << rhs.GetType().ToString();
                }
                if (lhs.GetType().IsPointer() &&
                    !((be->GetOp().GetKind() == TkPlus || be->GetOp().GetKind() == TkMinus) && 
                       rhs.GetType().GetTypeKind() >= ASTTypeKind::Char && rhs.GetType().GetTypeKind() <= ASTTypeKind::I64)) {
                    _diag.Report(be->GetStartLoc(), ErrUnsupportedTypeForOperator)
                        << llvm::SMRange(be->GetStartLoc(), be->GetEndLoc())
                        << be->GetOp().GetText()
                        << lhs.GetType().ToString()
                        << rhs.GetType().ToString();
                }
                else if ((lhs.GetType().IsPointer() && lhs.GetType().GetTypeKind() == ASTTypeKind::Noth ||
                          rhs.GetType().IsPointer()) && rhs.GetType().GetTypeKind() == ASTTypeKind::Noth) {
                    _diag.Report(be->GetStartLoc(), ErrNothPtrArithmetic)
                        << llvm::SMRange(be->GetStartLoc(), be->GetEndLoc());
                }
                else if (!(lhs.GetType().GetTypeKind() >= ASTTypeKind::Char && lhs.GetType().GetTypeKind() <= ASTTypeKind::F64 &&
                           rhs.GetType().GetTypeKind() >= ASTTypeKind::Char && rhs.GetType().GetTypeKind() <= ASTTypeKind::F64)) {
                    _diag.Report(be->GetStartLoc(), ErrUnsupportedTypeForOperator)
                        << llvm::SMRange(be->GetStartLoc(), be->GetEndLoc())
                        << be->GetOp().GetText()
                        << lhs.GetType().ToString()
                        << rhs.GetType().ToString();
                }
                break;
            case TkLogAnd:
            case TkLogOr:
                if (lhs.GetType().IsPointer() || rhs.GetType().IsPointer() ||
                    lhs.GetType().GetTypeKind() != ASTTypeKind::Bool ||
                    rhs.GetType().GetTypeKind() != ASTTypeKind::Bool) {
                    _diag.Report(be->GetStartLoc(), ErrUnsupportedTypeForOperator)
                        << llvm::SMRange(be->GetStartLoc(), be->GetEndLoc())
                        << be->GetOp().GetText()
                        << lhs.GetType().ToString()
                        << rhs.GetType().ToString();
                }
                break;
            case TkAnd:
            case TkOr:
                if (!(!lhs.GetType().IsPointer() && !rhs.GetType().IsPointer() &&
                      lhs.GetType().GetTypeKind() >= ASTTypeKind::Char && lhs.GetType().GetTypeKind() <= ASTTypeKind::I64 &&
                      rhs.GetType().GetTypeKind() >= ASTTypeKind::Char && rhs.GetType().GetTypeKind() <= ASTTypeKind::I64)) {
                    _diag.Report(be->GetStartLoc(), ErrUnsupportedTypeForOperator)
                        << llvm::SMRange(be->GetStartLoc(), be->GetEndLoc())
                        << be->GetOp().GetText()
                        << lhs.GetType().ToString()
                        << rhs.GetType().ToString();
                }
                break;
            case TkGt:
            case TkGtEq:
            case TkLt:
            case TkLtEq:
            case TkEqEq:
            case TkNotEq:
                if (lhs.GetType().IsPointer() && llvm::dyn_cast<NilExpr>(be->GetRHS()) &&
                    (be->GetOp().GetKind() == TkEqEq || be->GetOp().GetKind() == TkNotEq)) {
                    break;
                }
                else if (!ASTType::HasCommon(lhs.GetType(), rhs.GetType())) {
                    _diag.Report(be->GetStartLoc(), ErrUnsupportedTypeForOperator)
                        << llvm::SMRange(be->GetStartLoc(), be->GetEndLoc())
                        << be->GetOp().GetText()
                        << lhs.GetType().ToString()
                        << rhs.GetType().ToString();
                }
                break;
            default: {}
        }
    }

    bool
    SemanticAnalyzer::variableExists(std::string name) const {
        auto varsCopy = _vars;
        while (!varsCopy.empty()) {
            if (auto var = varsCopy.top().find(name); var != varsCopy.top().end()) {
                return true;
            }
            varsCopy.pop();
        }
        return false;
    }

    bool
    SemanticAnalyzer::canImplicitlyCast(ASTVal src, ASTType expectType, llvm::SMLoc startLoc, llvm::SMLoc endLoc) {
        resolveType(src.GetType(), startLoc, endLoc);
        resolveType(expectType, startLoc, endLoc);
        if (src.GetType() == expectType) {
            return true;
        }
        if (src.IsNil() && expectType.IsPointer() && src.GetType() == expectType) {
            return true;
        }
        if (!expectType.IsPointer() && expectType.GetTypeKind() == ASTTypeKind::Trait && src.GetType().GetTypeKind() == ASTTypeKind::Struct) {
            Struct *s = src.GetType().GetModule()->FindStruct(src.GetType().GetVal());
            if (s->TraitsImplements.find(expectType.GetVal()) != s->TraitsImplements.end()) {
                return true;
            }
        }
        if (auto it = implicitlyCastAllowed.find(src.GetType().GetTypeKind()); !src.GetType().IsPointer() && !expectType.IsPointer() && it != implicitlyCastAllowed.end()) {
            return std::find(it->second.begin(), it->second.end(), expectType.GetTypeKind()) != it->second.end();
        }
        return false;
    }
    
    ASTVal
    SemanticAnalyzer::implicitlyCast(ASTVal src, ASTType expectType, llvm::SMLoc startLoc, llvm::SMLoc endLoc) {
        resolveType(src.GetType(), startLoc, endLoc);
        resolveType(expectType, startLoc, endLoc);
        if (src.GetType() == expectType) {
            return src;
        }
        if (src.IsNil() && expectType.IsPointer() && (src.GetType() == expectType || src.GetType().GetTypeKind() == ASTTypeKind::Nil)) {
            return src;
        }
        if (!expectType.IsPointer() && expectType.GetTypeKind() == ASTTypeKind::Trait && src.GetType().GetTypeKind() == ASTTypeKind::Struct) {
            Struct *s = src.GetType().GetModule()->FindStruct(src.GetType().GetVal());
            if (s->TraitsImplements.find(expectType.GetVal()) != s->TraitsImplements.end()) {
                return ASTVal::GetVal(0, expectType);
            }
        }
        if (auto it = implicitlyCastAllowed.find(src.GetType().GetTypeKind()); !src.GetType().IsPointer() && !expectType.IsPointer() && it != implicitlyCastAllowed.end()) {
            if (std::find(it->second.begin(), it->second.end(), expectType.GetTypeKind()) != it->second.end()) {
                return src.Cast(expectType);
            }
        }
        _diag.Report(startLoc, ErrCannotCast)
            << llvm::SMRange(startLoc, endLoc)
            << src.GetType().ToString()
            << expectType.ToString();
        return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
    }
}

static std::vector<std::string>
splitString(std::string src, char separator) {
    if (src.empty()) {
        return {};
    }
    std::vector<std::string> res { "" };
    for (auto c : src) {
        if (c == separator) {
            res.push_back("");
            continue;
        }
        if (isspace(c)) {
            continue;
        }
        res.back() += c;
    }
    return res;
}
