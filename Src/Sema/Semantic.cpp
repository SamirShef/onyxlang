#include <onyx/Sema/Semantic.h>
#include <cmath>

namespace onyx {
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
                if (_functions.find(fds->GetName().str()) != _functions.end()) {
                    _diag.Report(llvm::SMLoc::getFromPointer(fds->GetName().data()), ErrRedefinitionFun)
                        << getRange(llvm::SMLoc::getFromPointer(fds->GetName().data()), fds->GetName().size())
                        << fds->GetName();
                    continue;
                }
                Function fun { .Name = fds->GetName(), .RetType = fds->GetRetType(), .Args = fds->GetArgs(), .Body = fds->GetBody() };
                _functions.emplace(fds->GetName().str(), fun);
            }
        }
    }
    
    std::optional<ASTVal>
    SemanticAnalyzer::VisitVarDeclStmt(VarDeclStmt *vds) {
        if (vds->GetAccess() == AccessPub && _vars.size() != 1) {
            _diag.Report(vds->GetStartLoc(), ErrCannotHaveAccessBeHere)
                << llvm::SMRange(vds->GetStartLoc(), vds->GetEndLoc());
        }
        if (_vars.top().find(vds->GetName().str()) != _vars.top().end()) {
            _diag.Report(llvm::SMLoc::getFromPointer(vds->GetName().data()), ErrRedefinitionVar)
                << getRange(llvm::SMLoc::getFromPointer(vds->GetName().data()), vds->GetName().size())
                << vds->GetName();
        }
        else {
            std::optional<ASTVal> val = vds->GetExpr() != nullptr ? Visit(vds->GetExpr()) : ASTVal::GetDefaultByType(vds->GetType());
            if (vds->GetType().GetTypeKind() == ASTTypeKind::Struct && vds->GetExpr() == nullptr) {
                Struct s = _structs.at(vds->GetType().GetVal().str());
                _structsInstances.push_back(s);
                val = ASTVal(ASTType(ASTTypeKind::Struct, s.Name, false), ASTValData { .structInstanceIndex = (long)(_structsInstances.size() - 1) });
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
            if (auto var = varsCopy.top().find(vas->GetName().str()); var != varsCopy.top().end()) {
                if (var->second.IsConst) {
                    _diag.Report(vas->GetStartLoc(), ErrAssignmentConst)
                        << llvm::SMRange(vas->GetStartLoc(), vas->GetEndLoc());
                    return std::nullopt;
                }
                ASTVal val = Visit(vas->GetExpr()).value_or(ASTVal::GetDefaultByType(ASTType::GetNothType()));
                val = implicitlyCast(val, var->second.Type, vas->GetExpr()->GetStartLoc(), vas->GetExpr()->GetEndLoc());
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
            _vars.top().emplace(arg.GetName(), Variable { .Name = arg.GetName(), .Type = arg.GetType(), .Val = ASTVal::GetDefaultByType(arg.GetType()),
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
        implicitlyCast(cond.value(), ASTType(ASTTypeKind::Bool, "bool", false), ies->GetCondition()->GetStartLoc(), ies->GetCondition()->GetEndLoc());
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
        implicitlyCast(cond.value(), ASTType(ASTTypeKind::Bool, "bool", false), fls->GetCondition()->GetStartLoc(), fls->GetCondition()->GetEndLoc());
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
        if (ss->GetAccess() == AccessPub && _vars.size() != 1) {
            _diag.Report(ss->GetStartLoc(), ErrCannotHaveAccessBeHere)
                << llvm::SMRange(ss->GetStartLoc(), ss->GetEndLoc());
        }
        if (_structs.find(ss->GetName().str()) != _structs.end()) {
            _diag.Report(ss->GetStartLoc(), ErrRedefinitionStruct)
                << llvm::SMRange(ss->GetStartLoc(), ss->GetEndLoc())
                << ss->GetName();
            return std::nullopt;
        }

        Struct s { ss->GetName(), {}, {} };
        for (int i = 0; i < ss->GetBody().size(); ++i) {
            if (ss->GetBody()[i]->GetKind() != NkVarDeclStmt) {
                _diag.Report(ss->GetStartLoc(), ErrCannotBeHere)
                    << llvm::SMRange(ss->GetStartLoc(), ss->GetEndLoc());
                continue;
            }

            VarDeclStmt *vds = llvm::dyn_cast<VarDeclStmt>(ss->GetBody()[i]);
            if (s.Fields.find(vds->GetName().str()) != s.Fields.end()) {
                _diag.Report(vds->GetStartLoc(), ErrRedefinitionField)
                    << llvm::SMRange(vds->GetStartLoc(), vds->GetEndLoc())
                    << vds->GetName();
                continue;
            }
            std::optional<ASTVal> val = vds->GetExpr() != nullptr ? Visit(vds->GetExpr()) : ASTVal::GetDefaultByType(vds->GetType());
            if (vds->GetExpr()) {
                implicitlyCast(val.value(), vds->GetType(), vds->GetExpr()->GetStartLoc(), vds->GetExpr()->GetEndLoc());
            }
            s.Fields.emplace(vds->GetName().str(), Field { vds->GetName(), val, vds->GetType(), vds->GetAccess(), false });
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
            Struct s = _structs.at(obj->GetType().GetVal().str());
            auto field = s.Fields.find(fas->GetName().str());
            if (field == s.Fields.end()) {
                _diag.Report(fas->GetStartLoc(), ErrUndeclaredField)
                    << llvm::SMRange(fas->GetStartLoc(), fas->GetEndLoc())
                    << fas->GetName()
                    << s.Name;
            }
            else {
                if (field->second.Access == AccessPriv) {
                    _diag.Report(fas->GetStartLoc(), ErrFieldIsPrivate)
                        << llvm::SMRange(fas->GetStartLoc(), fas->GetEndLoc())
                        << fas->GetName();
                }
                implicitlyCast(Visit(fas->GetExpr()).value(), s.Fields.at(fas->GetName().str()).Type, fas->GetStartLoc(), fas->GetEndLoc());
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
        Struct s = _structs.at(is->GetStructName().str());
        std::vector<FunDeclStmt *> methods;
        for (auto &stmt : is->GetBody()) {
            if (stmt->GetKind() != NkFunDeclStmt) {
                _diag.Report(stmt->GetStartLoc(), ErrCannotBeHere)
                    << llvm::SMRange(stmt->GetStartLoc(), stmt->GetEndLoc());
                continue;
            }
            FunDeclStmt *method = llvm::dyn_cast<FunDeclStmt>(stmt);
            if (s.Methods.find(method->GetName().str()) != s.Methods.end()) {
                _diag.Report(stmt->GetStartLoc(), ErrRedefinitionMethod)
                    << llvm::SMRange(stmt->GetStartLoc(), stmt->GetEndLoc())
                    << method->GetName();
            }
            else {
                methods.push_back(method);
                Function fun { .Name = method->GetName(), .RetType = method->GetRetType(), .Args = method->GetArgs(), .Body = method->GetBody() };
                s.Methods.emplace(method->GetName().str(), Method { .Fun = fun, .Access = method->GetAccess() });
            }
        }
        _structs.at(is->GetStructName().str()).Methods = s.Methods;

        for (auto &method : methods) {
            _vars.push({});
            ASTType thisType = ASTType(ASTTypeKind::Struct, s.Name.str(), false);
            _vars.top().emplace("this", Variable { .Name = "this", .Type = thisType, .Val = ASTVal::GetDefaultByType(thisType),  // TODO: create real logic for .Val
                                                   .IsConst = false });
            for (auto arg : method->GetArgs()) {
                _vars.top().emplace(arg.GetName(), Variable { .Name = arg.GetName(), .Type = arg.GetType(), .Val = ASTVal::GetDefaultByType(arg.GetType()),
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
            return ASTVal::GetVal(res, ASTType(ASTTypeKind::Bool, "bool", false));
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
            if (auto var = varsCopy.top().find(ve->GetName().str()); var != varsCopy.top().end()) {
                return var->second.Val;
            }
            varsCopy.pop();
        }
        _diag.Report(ve->GetStartLoc(), ErrUndeclaredVariable)
            << getRange(ve->GetStartLoc(), ve->GetName().size())
            << ve->GetName();
        return ASTVal(ASTType(ASTTypeKind::I32, "i32", false), ASTValData { .i32Val = 0 });
    }
    
    std::optional<ASTVal>
    SemanticAnalyzer::VisitLiteralExpr(LiteralExpr *le) {
        return le->GetVal();
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitFunCallExpr(FunCallExpr *fce) {
        if (_functions.find(fce->GetName().str()) != _functions.end()) {
            _vars.push({});
            Function fun = _functions.at(fce->GetName().str());
            if (fun.Args.size() != fce->GetArgs().size()) {
                _diag.Report(fce->GetStartLoc(), ErrFewArgs)
                    << llvm::SMRange(fce->GetStartLoc(), fce->GetEndLoc())
                    << fce->GetName().str()
                    << fun.Args.size()
                    << fce->GetArgs().size();
                return std::nullopt;
            }
            for (int i = 0; i < fun.Args.size(); ++i) {
                std::optional<ASTVal> val = Visit(fce->GetArgs()[i]);
                implicitlyCast(val.value_or(ASTVal::GetDefaultByType(ASTType::GetNothType())), fun.Args[i].GetType(),
                               fce->GetArgs()[i]->GetStartLoc(), fce->GetArgs()[i]->GetEndLoc());
                _vars.top().emplace(fun.Args[i].GetName(), Variable { .Name = fun.Args[i].GetName(), .Type = fun.Args[i].GetType(),
                                                                      .Val = val, .IsConst = fun.Args[i].GetType().IsConst() });
            }
            for (auto stmt : fun.Body) {
                if (stmt->GetKind() == NkRetStmt) {
                    Expr *expr = llvm::dyn_cast<RetStmt>(stmt)->GetExpr();
                    std::optional<ASTVal> val = expr ? Visit(expr) : ASTVal::GetDefaultByType(ASTType::GetNothType());
                    implicitlyCast(val.value_or(ASTVal::GetDefaultByType(ASTType::GetNothType())), fun.RetType, fce->GetStartLoc(),
                                   fce->GetEndLoc());
                    _vars.pop();
                    return val;
                }
                else {
                    Visit(stmt);
                }
            }
            _vars.pop();
            _diag.Report(fce->GetStartLoc(), ErrFuntionCannotReturnValue);
            return ASTVal(ASTType(ASTTypeKind::I32, "i32", false), ASTValData { .i32Val = 0 });
        }
        _diag.Report(fce->GetStartLoc(), ErrUndeclaredFuntion)
            << getRange(fce->GetStartLoc(), fce->GetName().size())
            << fce->GetName();
        return ASTVal(ASTType(ASTTypeKind::I32, "i32", false), ASTValData { .i32Val = 0 });
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitStructExpr(StructExpr *se) {
        if (_structs.find(se->GetName().str()) == _structs.end()) {
            _diag.Report(se->GetStartLoc(), ErrUndeclaredStructure)
                << llvm::SMRange(se->GetStartLoc(), se->GetEndLoc())
                << se->GetName();
            return ASTVal(ASTType(ASTTypeKind::I32, "i32", false), ASTValData { .i32Val = 0 });
        }
        Struct s = _structs.at(se->GetName().str());
        for (int i = 0; i < se->GetInitializer().size(); ++i) {
            std::string name = se->GetInitializer()[i].first.str();
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
        _structsInstances.push_back(s);
        return ASTVal(ASTType(ASTTypeKind::Struct, s.Name, false), ASTValData { .structInstanceIndex = (long)(_structsInstances.size() - 1) });
    }


    std::optional<ASTVal>
    SemanticAnalyzer::VisitFieldAccessExpr(FieldAccessExpr *fae) {
        std::optional<ASTVal> obj = Visit(fae->GetObject());
        if (obj->GetType().GetTypeKind() != ASTTypeKind::Struct) {
            _diag.Report(fae->GetStartLoc(), ErrAccessFromNonStruct)
                << llvm::SMRange(fae->GetStartLoc(), fae->GetEndLoc());
        }
        else {
            Struct s = _structs.at(obj->GetType().GetVal().str());
            auto field = s.Fields.find(fae->GetName().str());
            if (field == s.Fields.end()) {
                _diag.Report(fae->GetStartLoc(), ErrUndeclaredField)
                    << llvm::SMRange(fae->GetStartLoc(), fae->GetEndLoc())
                    << fae->GetName()
                    << s.Name;
            }
            else {
                if (field->second.Access == AccessPriv) {
                    _diag.Report(fae->GetStartLoc(), ErrFieldIsPrivate)
                        << llvm::SMRange(fae->GetStartLoc(), fae->GetEndLoc())
                        << fae->GetName();
                }
                return s.Fields.at(fae->GetName().str()).Val;
            }
        }
        return ASTVal(ASTType(ASTTypeKind::I32, "i32", false), ASTValData { .i32Val = 0 });
    }

    std::optional<ASTVal>
    SemanticAnalyzer::VisitMethodCallExpr(MethodCallExpr *mce) {
        std::optional<ASTVal> obj = Visit(mce->GetObject());
        if (obj->GetType().GetTypeKind() != ASTTypeKind::Struct) {
            _diag.Report(mce->GetStartLoc(), ErrAccessFromNonStruct)
                << llvm::SMRange(mce->GetStartLoc(), mce->GetEndLoc());
        }
        else {
            Struct s = _structs.at(obj->GetType().GetVal().str());
            auto method = s.Methods.find(mce->GetName().str());
            if (method == s.Methods.end()) {
                _diag.Report(mce->GetStartLoc(), ErrUndeclaredMethod)
                    << llvm::SMRange(mce->GetStartLoc(), mce->GetEndLoc())
                    << mce->GetName()
                    << s.Name;
            }
            else {
                if (method->second.Access == AccessPriv) {
                    _diag.Report(mce->GetStartLoc(), ErrMethodIsPrivate)
                        << llvm::SMRange(mce->GetStartLoc(), mce->GetEndLoc())
                        << mce->GetName();
                }

                _vars.push({});
                if (method->second.Fun.Args.size() != mce->GetArgs().size()) {
                    _diag.Report(mce->GetStartLoc(), ErrFewArgs)
                        << llvm::SMRange(mce->GetStartLoc(), mce->GetEndLoc())
                        << mce->GetName().str()
                        << method->second.Fun.Args.size()
                        << mce->GetArgs().size();
                    return std::nullopt;
                }
                for (int i = 0; i < method->second.Fun.Args.size(); ++i) {
                    std::optional<ASTVal> val = Visit(mce->GetArgs()[i]);
                    implicitlyCast(val.value_or(ASTVal::GetDefaultByType(ASTType::GetNothType())), method->second.Fun.Args[i].GetType(),
                                   mce->GetArgs()[i]->GetStartLoc(), mce->GetArgs()[i]->GetEndLoc());
                    _vars.top().emplace(method->second.Fun.Args[i].GetName(), Variable { .Name = method->second.Fun.Args[i].GetName(),
                                        .Type = method->second.Fun.Args[i].GetType(), .Val = val, .IsConst = method->second.Fun.Args[i].GetType().IsConst() });
                }
                for (auto stmt : method->second.Fun.Body) {
                    if (stmt->GetKind() == NkRetStmt) {
                        Expr *expr = llvm::dyn_cast<RetStmt>(stmt)->GetExpr();
                        std::optional<ASTVal> val = expr ? Visit(expr) : ASTVal::GetDefaultByType(ASTType::GetNothType());
                        implicitlyCast(val.value_or(ASTVal::GetDefaultByType(ASTType::GetNothType())), method->second.Fun.RetType, mce->GetStartLoc(),
                                       mce->GetEndLoc());
                        _vars.pop();
                        return val;
                    }
                    else {
                        Visit(stmt);
                    }
                }
                _vars.pop();
                _diag.Report(mce->GetStartLoc(), ErrMethodCannotReturnValue);
                return ASTVal(ASTType(ASTTypeKind::I32, "i32", false), ASTValData { .i32Val = 0 });
            }
        }
        return ASTVal(ASTType(ASTTypeKind::I32, "i32", false), ASTValData { .i32Val = 0 });
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
                if (!(lhs.GetType().GetTypeKind() >= ASTTypeKind::Char && lhs.GetType().GetTypeKind() <= ASTTypeKind::F64 &&
                    rhs.GetType().GetTypeKind() >= ASTTypeKind::Char && rhs.GetType().GetTypeKind() <= ASTTypeKind::F64)) {
                    _diag.Report(be->GetStartLoc(), ErrUnsupportedTypeForOperator)
                        << llvm::SMRange(be->GetStartLoc(), be->GetEndLoc())
                        << be->GetOp().GetText().str()
                        << lhs.GetType().ToString()
                        << rhs.GetType().ToString();
                }
                break;
            case TkLogAnd:
            case TkLogOr:
                if (lhs.GetType().GetTypeKind() != ASTTypeKind::Bool ||
                    rhs.GetType().GetTypeKind() != ASTTypeKind::Bool) {
                    _diag.Report(be->GetStartLoc(), ErrUnsupportedTypeForOperator)
                        << llvm::SMRange(be->GetStartLoc(), be->GetEndLoc())
                        << be->GetOp().GetText().str()
                        << lhs.GetType().ToString()
                        << rhs.GetType().ToString();
                }
                break;
            case TkAnd:
            case TkOr:
                if (!(lhs.GetType().GetTypeKind() >= ASTTypeKind::Char && lhs.GetType().GetTypeKind() <= ASTTypeKind::I64 &&
                    rhs.GetType().GetTypeKind() >= ASTTypeKind::Char && rhs.GetType().GetTypeKind() <= ASTTypeKind::I64)) {
                    _diag.Report(be->GetStartLoc(), ErrUnsupportedTypeForOperator)
                        << llvm::SMRange(be->GetStartLoc(), be->GetEndLoc())
                        << be->GetOp().GetText().str()
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
                if (!ASTType::HasCommon(lhs.GetType(), rhs.GetType())) {
                    _diag.Report(be->GetStartLoc(), ErrUnsupportedTypeForOperator)
                        << llvm::SMRange(be->GetStartLoc(), be->GetEndLoc())
                        << be->GetOp().GetText().str()
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
        if (auto it = implicitlyCastAllowed.find(src.GetType().GetTypeKind()); it != implicitlyCastAllowed.end()) {
            return std::find(it->second.begin(), it->second.end(), expectType.GetTypeKind()) != it->second.end();
        }
        return false;
    }
    
    ASTVal
    SemanticAnalyzer::implicitlyCast(ASTVal src, ASTType expectType, llvm::SMLoc startLoc, llvm::SMLoc endLoc) const {
        if (src.GetType() == expectType) {
            return src;
        }
        if (auto it = implicitlyCastAllowed.find(src.GetType().GetTypeKind()); it != implicitlyCastAllowed.end()) {
            if (std::find(it->second.begin(), it->second.end(), expectType.GetTypeKind()) != it->second.end()) {
                return src.Cast(expectType);
            }
        }
        _diag.Report(startLoc, ErrCannotCast)
            << llvm::SMRange(startLoc, endLoc)
            << src.GetType().ToString()
            << expectType.ToString();
        return ASTVal::GetDefaultByType(ASTType::GetNothType());
    }
}
