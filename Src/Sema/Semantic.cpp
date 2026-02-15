#include <marble/Sema/Semantic.h>
#include <cmath>

namespace marble {
    static std::unordered_map<ASTTypeKind, std::vector<ASTTypeKind>> implicitlyCastAllowed {
        { ASTTypeKind::Char, { ASTTypeKind::I16, ASTTypeKind::I32, ASTTypeKind::I64, ASTTypeKind::F32, ASTTypeKind::F64 } },
        { ASTTypeKind::I16,  { ASTTypeKind::I32, ASTTypeKind::I64, ASTTypeKind::F32, ASTTypeKind::F64                   } },
        { ASTTypeKind::I32,  { ASTTypeKind::I64, ASTTypeKind::F32, ASTTypeKind::F64                                     } },
        { ASTTypeKind::I64,  { ASTTypeKind::F32, ASTTypeKind::F64                                                       } },
        { ASTTypeKind::F32,  { ASTTypeKind::F64                                                                         } },
    };

    void
    SemanticAnalyzer::DeclareFunctions(std::vector<Stmt *> &ast) {
        for (auto &stmt : ast) {
            if (stmt->GetKind() == NkFunDeclStmt) {
                FunDeclStmt *fds = llvm::dyn_cast<FunDeclStmt>(stmt);
                if (fds->IsDeclaration()) {
                    _diag.Report(fds->GetStartLoc(), ErrCannotDeclareHere)
                        << llvm::SMRange(fds->GetStartLoc(), fds->GetEndLoc());
                    continue;
                }
                if (_functions.find(fds->GetName()) != _functions.end()) {
                    _diag.Report(llvm::SMLoc::getFromPointer(fds->GetName().data()), ErrRedefinitionFun)
                        << getRange(llvm::SMLoc::getFromPointer(fds->GetName().data()), fds->GetName().size())
                        << fds->GetName();
                    continue;
                }
                Function fun { .Name = fds->GetName(), .RetType = fds->GetRetType(), .Args = fds->GetArgs(), .Body = fds->GetBody(), .IsDeclaration = fds->IsDeclaration() };
                _functions.emplace(fds->GetName(), fun);
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
            _diag.Report(llvm::SMLoc::getFromPointer(vds->GetName().data()), ErrRedefinitionVar)
                << getRange(llvm::SMLoc::getFromPointer(vds->GetName().data()), vds->GetName().size())
                << vds->GetName();
        }
        else {
            std::optional<ASTVal> val = vds->GetExpr() != nullptr ? Visit(vds->GetExpr()) : ASTVal::GetDefaultByType(vds->GetType());
            if (vds->GetType().GetTypeKind() == ASTTypeKind::Struct && vds->GetExpr() == nullptr) {
                Struct s = _structs.at(vds->GetType().GetVal());
                val = ASTVal(ASTType(ASTTypeKind::Struct, s.Name, false, 0), ASTValData { .i32Val = 0 }, false, false);
            }
            Variable var { .Name = vds->GetName(), .Type = vds->GetType(), .Val = val, .IsConst = vds->IsConst() };
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
        for (auto arg : fds->GetArgs()) {
            if (_vars.top().find(arg.GetName()) != _vars.top().end()) {
                _diag.Report(fds->GetStartLoc(), ErrRedefinitionVar)
                    << llvm::SMRange(fds->GetStartLoc(), fds->GetEndLoc())
                    << arg.GetName();
            }
            _vars.top().emplace(arg.GetName(), Variable { .Name = arg.GetName(), .Type = arg.GetType(),
                                                          .Val = arg.GetType().IsPointer() ? ASTVal(arg.GetType(), ASTValData { .i32Val = 0 }, false, false)
                                                                                           : ASTVal::GetDefaultByType(arg.GetType()),
                                                          .IsConst = arg.GetType().IsConst() });
        }
        _funRetsTypes.push(fds->GetRetType());
        bool hasRet;
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
        if (_structs.find(ss->GetName()) != _structs.end()) {
            _diag.Report(ss->GetStartLoc(), ErrRedefinitionStruct)
                << llvm::SMRange(ss->GetStartLoc(), ss->GetEndLoc())
                << ss->GetName();
            return std::nullopt;
        }

        Struct s { .Name = ss->GetName(), .Fields = {}, .Methods = {}, .TraitsImplements = {} };
        for (int i = 0; i < ss->GetBody().size(); ++i) {
            if (ss->GetBody()[i]->GetKind() != NkVarDeclStmt) {
                _diag.Report(ss->GetStartLoc(), ErrCannotBeHere)
                    << llvm::SMRange(ss->GetStartLoc(), ss->GetEndLoc());
                continue;
            }

            VarDeclStmt *vds = llvm::dyn_cast<VarDeclStmt>(ss->GetBody()[i]);
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
            s.Fields.emplace(vds->GetName(), Field { .Name = vds->GetName(), .Val = val, .Type = vds->GetType(), .IsConst = vds->IsConst(), .Access = vds->GetAccess(),
                             .ManualInitialized = false });
        }
        _structs.emplace(s.Name, s);

        return std::nullopt;
    }
    
    std::optional<ASTVal>
    SemanticAnalyzer::VisitFieldAsgnStmt(FieldAsgnStmt *fas) {
        if (_vars.size() == 1) {
            _diag.Report(fas->GetStartLoc(), ErrCannotBeHere)
                << llvm::SMRange(fas->GetStartLoc(), fas->GetEndLoc());
        }
        std::optional<ASTVal> obj = Visit(fas->GetObject());
        if (obj->GetType().GetTypeKind() != ASTTypeKind::Struct) {
            _diag.Report(fas->GetStartLoc(), ErrAccessFromNonStruct)
                << llvm::SMRange(fas->GetStartLoc(), fas->GetEndLoc());
        }
        else {
            fas->SetObjType(obj->GetType());
            bool objIsThis = false;
            if (fas->GetObject()->GetKind() == NkVarExpr) {
                VarExpr *ve = llvm::cast<VarExpr>(fas->GetObject());
                if (ve->GetName() == "this") {
                    objIsThis = true;
                }
            }
            Struct s = _structs.at(obj->GetType().GetVal());
            auto field = s.Fields.find(fas->GetName());
            if (field == s.Fields.end()) {
                _diag.Report(fas->GetStartLoc(), ErrUndeclaredField)
                    << llvm::SMRange(fas->GetStartLoc(), fas->GetEndLoc())
                    << fas->GetName()
                    << s.Name;
            }
            else {
                if (field->second.Access == AccessPriv && !objIsThis) {
                    _diag.Report(fas->GetStartLoc(), ErrFieldIsPrivate)
                        << llvm::SMRange(fas->GetStartLoc(), fas->GetEndLoc())
                        << fas->GetName();
                }
                if (field->second.IsConst) {
                    _diag.Report(fas->GetStartLoc(), ErrAssignmentConst)
                        << llvm::SMRange(fas->GetStartLoc(), fas->GetEndLoc());
                    return std::nullopt;
                }
                implicitlyCast(Visit(fas->GetExpr()).value(), s.Fields.at(fas->GetName()).Type, fas->GetStartLoc(), fas->GetEndLoc());
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
        if (_structs.find(is->GetStructName()) == _structs.end()) {
            _diag.Report(is->GetStartLoc(), ErrUndeclaredStructure)
                 << llvm::SMRange(is->GetStartLoc(), is->GetEndLoc())
                 << is->GetStructName();
            return std::nullopt;
        }
        Struct s = _structs.at(is->GetStructName());
        bool isTraitImpl = !is->GetTraitName().empty();
        const Trait *traitDef = nullptr;
        std::unordered_map<std::string, bool> implementedTraitMethods;
        
        if (isTraitImpl) {
            auto tIt = _traits.find(is->GetTraitName());
            if (tIt == _traits.end()) {
                _diag.Report(is->GetStartLoc(), ErrUndeclaredTrait)
                    << llvm::SMRange(is->GetStartLoc(), is->GetEndLoc())
                    << is->GetTraitName();
                return std::nullopt;
            }
            traitDef = &tIt->second;
            
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
            FunDeclStmt *method = llvm::dyn_cast<FunDeclStmt>(stmt);
            if (method->IsDeclaration()) {
                _diag.Report(method->GetStartLoc(), ErrCannotDeclareHere)
                    << llvm::SMRange(method->GetStartLoc(), method->GetEndLoc());
                continue;
            }
            if (s.Methods.find(method->GetName()) != s.Methods.end()) {
                _diag.Report(stmt->GetStartLoc(), ErrRedefinitionMethod)
                    << llvm::SMRange(stmt->GetStartLoc(), stmt->GetEndLoc())
                    << method->GetName();
                continue;
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

                const Function &traitFun = tMethodIt->second.Fun;
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
                                << method->GetArgs()[i].GetName()
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
                           .IsDeclaration = method->IsDeclaration() };
            s.Methods.emplace(method->GetName(), Method { .Fun = fun, .Access = method->GetAccess() });
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
            s.TraitsImplements.emplace(traitDef->Name, *traitDef);
        }
        _structs.at(is->GetStructName()).Methods = s.Methods;
        _structs.at(is->GetStructName()).TraitsImplements = s.TraitsImplements;

        for (auto &method : methods) {
            _vars.push({});
            ASTType thisType = ASTType(ASTTypeKind::Struct, s.Name, false, 0);
            _vars.top().emplace("this", Variable { .Name = "this", .Type = thisType, .Val = ASTVal::GetDefaultByType(thisType), .IsConst = false });
            for (auto arg : method->GetArgs()) {
                if (_vars.top().find(arg.GetName()) != _vars.top().end()) {
                    _diag.Report(method->GetStartLoc(), ErrRedefinitionVar)
                        << llvm::SMRange(method->GetStartLoc(), method->GetEndLoc())
                        << arg.GetName();
                }
                _vars.top().emplace(arg.GetName(), Variable { .Name = arg.GetName(), .Type = arg.GetType(),
                                                              .Val = arg.GetType().IsPointer() ? ASTVal(arg.GetType(), ASTValData { .i32Val = 0 }, false, false)
                                                                                               : ASTVal::GetDefaultByType(arg.GetType()),
                                                              .IsConst = arg.GetType().IsConst() });
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
        mcs->SetObjType(Visit(mcs->GetObject())->GetType());
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
        if (_traits.find(tds->GetName()) != _traits.end()) {
            _diag.Report(tds->GetStartLoc(), ErrRedefinitionTrait)
                << llvm::SMRange(tds->GetStartLoc(), tds->GetEndLoc())
                << tds->GetName();
        }
        Trait t { .Name = tds->GetName(), .Methods = {} };
        for (auto stmt : tds->GetBody()) {
            if (FunDeclStmt *method = llvm::dyn_cast<FunDeclStmt>(stmt)) {
                if (!method->IsDeclaration()) {
                    _diag.Report(method->GetStartLoc(), ErrExpectedDeclarationInTrait)
                        << llvm::SMRange(method->GetStartLoc(), method->GetEndLoc())
                        << method->GetName()
                        << tds->GetName();
                }
                Function fun { .Name = method->GetName(), .RetType = method->GetRetType(), .Args = method->GetArgs(), .Body = method->GetBody(),
                               .IsDeclaration = method->IsDeclaration() };
                t.Methods.emplace(method->GetName(), Method { .Fun = fun, .Access = method->GetAccess() });
            }
            else {
                _diag.Report(stmt->GetStartLoc(), ErrCannotBeHere)
                    << llvm::SMRange(stmt->GetStartLoc(), stmt->GetEndLoc());
            }
        }
        _traits.emplace(tds->GetName(), t);
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
        return Visit(es->GetRHS());
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitDelStmt(DelStmt *ds) {
        if (_vars.size() == 1) {
            _diag.Report(ds->GetStartLoc(), ErrCannotBeHere)
                << llvm::SMRange(ds->GetStartLoc(), ds->GetEndLoc());
        }
        std::optional<ASTVal> val = Visit(ds->GetExpr());
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
                if (var->second.Type.GetTypeKind() == ASTTypeKind::Trait) {
                    return ASTVal(var->second.Type, ASTValData { .i32Val = 0 }, var->second.Val->IsNil(), var->second.Val->CreatedByNew());
                }
                return ASTVal(var->second.Type, var->second.Val->GetData(), var->second.Val->IsNil(), var->second.Val->CreatedByNew());
            }
            varsCopy.pop();
        }
        _diag.Report(ve->GetStartLoc(), ErrUndeclaredVariable)
            << getRange(ve->GetStartLoc(), ve->GetName().size())
            << ve->GetName();
        return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
    }
    
    std::optional<ASTVal>
    SemanticAnalyzer::VisitLiteralExpr(LiteralExpr *le) {
        return le->GetVal();
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitFunCallExpr(FunCallExpr *fce) {
        if (_functions.find(fce->GetName()) != _functions.end()) {
            Function fun = _functions.at(fce->GetName());
            if (fun.Args.size() != fce->GetArgs().size()) {
                _diag.Report(fce->GetStartLoc(), ErrFewArgs)
                    << llvm::SMRange(fce->GetStartLoc(), fce->GetEndLoc())
                    << fce->GetName()
                    << fun.Args.size()
                    << fce->GetArgs().size();
                return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
            }
            for (int i = 0; i < fun.Args.size(); ++i) {
                implicitlyCast(Visit(fce->GetArgs()[i]).value(), fun.Args[i].GetType(), fce->GetArgs()[i]->GetStartLoc(), fce->GetArgs()[i]->GetEndLoc());
            }
            if (fun.RetType.GetTypeKind() != ASTTypeKind::Noth) {
                return ASTVal::GetDefaultByType(fun.RetType);
            }
            return ASTVal::GetDefaultByType(ASTType::GetNothType());
        }
        _diag.Report(fce->GetStartLoc(), ErrUndeclaredFuntion)
            << getRange(fce->GetStartLoc(), fce->GetName().size())
            << fce->GetName();
        return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitStructExpr(StructExpr *se) {
        if (_structs.find(se->GetName()) == _structs.end()) {
            _diag.Report(se->GetStartLoc(), ErrUndeclaredStructure)
                << llvm::SMRange(se->GetStartLoc(), se->GetEndLoc())
                << se->GetName();
            return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
        }
        Struct s = _structs.at(se->GetName());
        for (int i = 0; i < se->GetInitializer().size(); ++i) {
            std::string name = se->GetInitializer()[i].first;
            if (s.Fields.find(name) != s.Fields.end()) {
                if (s.Fields.at(name).ManualInitialized) {
                    _diag.Report(se->GetStartLoc(), ErrFieldInitialized)
                        << llvm::SMRange(se->GetStartLoc(), se->GetEndLoc())
                        << name;
                }
                else {
                    s.Fields.at(name).Val = Visit(se->GetInitializer()[i].second);
                    s.Fields.at(name).ManualInitialized = true;
                }
            }
            else {
                _diag.Report(se->GetStartLoc(), ErrUndeclaredField)
                    << llvm::SMRange(se->GetStartLoc(), se->GetEndLoc())
                    << name
                    << s.Name;
            }
        }
        return ASTVal(ASTType(ASTTypeKind::Struct, s.Name, false, 0), ASTValData { .i32Val = 0 }, false, false);
    }


    std::optional<ASTVal>
    SemanticAnalyzer::VisitFieldAccessExpr(FieldAccessExpr *fae) {
        std::optional<ASTVal> obj = Visit(fae->GetObject());
        if (obj->GetType().GetTypeKind() != ASTTypeKind::Struct) {
            _diag.Report(fae->GetStartLoc(), ErrAccessFromNonStruct)
                << llvm::SMRange(fae->GetStartLoc(), fae->GetEndLoc());
        }
        else {
            fae->SetObjType(obj->GetType());
            bool objIsThis = false;
            if (fae->GetObject()->GetKind() == NkVarExpr) {
                VarExpr *ve = llvm::cast<VarExpr>(fae->GetObject());
                if (ve->GetName() == "this") {
                    objIsThis = true;
                }
            }
            Struct s = _structs.at(obj->GetType().GetVal());
            auto field = s.Fields.find(fae->GetName());
            if (field == s.Fields.end()) {
                _diag.Report(fae->GetStartLoc(), ErrUndeclaredField)
                    << llvm::SMRange(fae->GetStartLoc(), fae->GetEndLoc())
                    << fae->GetName()
                    << s.Name;
            }
            else {
                if (field->second.Access == AccessPriv && !objIsThis) {
                    _diag.Report(fae->GetStartLoc(), ErrFieldIsPrivate)
                        << llvm::SMRange(fae->GetStartLoc(), fae->GetEndLoc())
                        << fae->GetName();
                }
                return s.Fields.at(fae->GetName()).Val;
            }
        }
        return ASTVal(ASTType(ASTTypeKind::I32, "i32", false, 0), ASTValData { .i32Val = 0 }, false, false);
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitMethodCallExpr(MethodCallExpr *mce) {
        std::optional<ASTVal> obj = Visit(mce->GetObject());
        if (obj->GetType().GetTypeKind() != ASTTypeKind::Struct &&
            obj->GetType().GetTypeKind() != ASTTypeKind::Trait) {
            _diag.Report(mce->GetStartLoc(), ErrAccessFromNonStruct)
                << llvm::SMRange(mce->GetStartLoc(), mce->GetEndLoc());
        }
        else {
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
                Struct &s = _structs.at(obj->GetType().GetVal());
                methods = &s.Methods;
                contextName = s.Name;
            }
            else {
                Trait &t = _traits.at(obj->GetType().GetVal());
                methods = &t.Methods;
                contextName = t.Name;
            }
            auto method = methods->find(mce->GetName());
            if (method == methods->end()) {
                _diag.Report(mce->GetStartLoc(), ErrUndeclaredMethod)
                    << llvm::SMRange(mce->GetStartLoc(), mce->GetEndLoc())
                    << mce->GetName()
                    << contextName;
            }
            else {
                if (method->second.Access == AccessPriv && !objIsThis) {
                    _diag.Report(mce->GetStartLoc(), ErrMethodIsPrivate)
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
                    implicitlyCast(Visit(mce->GetArgs()[i]).value(), method->second.Fun.Args[i].GetType(), mce->GetArgs()[i]->GetStartLoc(), mce->GetArgs()[i]->GetEndLoc());
                }
                if (method->second.Fun.RetType.GetTypeKind() != ASTTypeKind::Noth) {
                    return ASTVal::GetDefaultByType(method->second.Fun.RetType);
                }
                return ASTVal::GetDefaultByType(ASTType::GetNothType());
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
        de->SetExprType(val->GetType());
        return ASTVal(val->GetType().Deref(), val->GetData(), false, val->CreatedByNew());
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitRefExpr(RefExpr *re) {
        std::optional<ASTVal> val = Visit(re->GetExpr());
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
        return ASTVal(ne->GetType().Ref(), ASTValData { .i32Val = 0 }, false, true);
    }

    llvm::SMRange
    SemanticAnalyzer::getRange(llvm::SMLoc start, int len) const {
        return llvm::SMRange(start, llvm::SMLoc::getFromPointer(start.getPointer() + len));
    }

    void
    SemanticAnalyzer::checkBinaryExpr(BinaryExpr *be) {
        ASTVal lhs = Visit(be->GetLHS()).value();
        ASTVal rhs = Visit(be->GetRHS()).value();
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
    SemanticAnalyzer::canImplicitlyCast(ASTVal src, ASTType expectType) const {
        if (src.GetType() == expectType) {
            return true;
        }
        if (src.IsNil() && expectType.IsPointer()) {
            return true;
        }
        if (!expectType.IsPointer() && expectType.GetTypeKind() == ASTTypeKind::Trait && src.GetType().GetTypeKind() == ASTTypeKind::Struct) {
            Struct s = _structs.at(src.GetType().GetVal());
            if (s.TraitsImplements.find(expectType.GetVal()) != s.TraitsImplements.end()) {
                return true;
            }
        }
        if (auto it = implicitlyCastAllowed.find(src.GetType().GetTypeKind()); !src.GetType().IsPointer() && !expectType.IsPointer() && it != implicitlyCastAllowed.end()) {
            return std::find(it->second.begin(), it->second.end(), expectType.GetTypeKind()) != it->second.end();
        }
        return false;
    }
    
    ASTVal
    SemanticAnalyzer::implicitlyCast(ASTVal src, ASTType expectType, llvm::SMLoc startLoc, llvm::SMLoc endLoc) const {
        if (src.GetType() == expectType) {
            return src;
        }
        if (src.IsNil() && expectType.IsPointer()) {
            return src;
        }
        if (!expectType.IsPointer() && expectType.GetTypeKind() == ASTTypeKind::Trait && src.GetType().GetTypeKind() == ASTTypeKind::Struct) {
            Struct s = _structs.at(src.GetType().GetVal());
            if (s.TraitsImplements.find(expectType.GetVal()) != s.TraitsImplements.end()) {
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
