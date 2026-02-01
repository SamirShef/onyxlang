#include <onyx/CodeGen/CodeGen.h>

static bool createLoad = true;

namespace onyx {
    void
    CodeGen::DeclareFunctionsAndStructures(std::vector<Stmt *> &ast) {
        llvm::FunctionType *printfType = llvm::FunctionType::get(llvm::Type::getInt32Ty(_context), { llvm::PointerType::getInt8Ty(_context) }, true);
        llvm::Function *printfFun = llvm::Function::Create(printfType, llvm::GlobalValue::ExternalLinkage, "printf", *_module);

        for (auto &stmt : ast) {
            if (FunDeclStmt *fds = llvm::dyn_cast<FunDeclStmt>(stmt)) {
                std::vector<llvm::Type *> args(fds->GetArgs().size());
                std::vector<ASTType> argsAST(fds->GetArgs().size());
                for (int i = 0; i < fds->GetArgs().size(); ++i) {
                    args[i] = typeToLLVM(fds->GetArgs()[i].GetType());
                    argsAST[i] = fds->GetArgs()[i].GetType();
                }
                llvm::FunctionType *retType = llvm::FunctionType::get(typeToLLVM(fds->GetRetType()), args, false);
                llvm::Function *fun = llvm::Function::Create(retType, llvm::GlobalValue::ExternalLinkage, fds->GetName().str(), *_module);
                
                if (fds->GetRetType().GetTypeKind() == ASTTypeKind::Struct) {
                    llvm::MDNode *metadata = llvm::MDNode::get(_context, llvm::MDString::get(_context, fds->GetRetType().GetVal()));
                    fun->setMetadata("struct_name", metadata);
                }
                _functions.emplace(fun->getName(), fun);
                _funArgsTypes.emplace(fun->getName().str(), argsAST);
            }
            else if (ImplStmt *is = llvm::dyn_cast<ImplStmt>(stmt)) {
                Struct &s = _structs.at(is->GetStructName().str());
                if (!is->GetTraitName().empty()) {
                    s.TraitsImplements.emplace(is->GetTraitName().str(), _traits.at(is->GetTraitName().str()));
                }
                for (auto stmt : is->GetBody()) {
                    FunDeclStmt *fds = llvm::cast<FunDeclStmt>(stmt);
                    std::vector<llvm::Type *> args(fds->GetArgs().size() + 1);
                    std::vector<ASTType> argsAST(fds->GetArgs().size() + 1);
                    args[0] = llvm::PointerType::get(_structs.at(is->GetStructName().str()).Type, 0);
                    argsAST[0] = ASTType(ASTTypeKind::Struct, is->GetStructName().str(), true);
                    for (int i = 0; i < fds->GetArgs().size(); ++i) {
                        args[i + 1] = typeToLLVM(fds->GetArgs()[i].GetType());
                        argsAST[i + 1] = fds->GetArgs()[i].GetType();
                    }
                    llvm::FunctionType *retType = llvm::FunctionType::get(typeToLLVM(fds->GetRetType()), args, false);
                    llvm::Function *fun = llvm::Function::Create(retType, llvm::GlobalValue::ExternalLinkage, is->GetStructName() + "." + fds->GetName().str(), *_module);
                    
                    if (fds->GetRetType().GetTypeKind() == ASTTypeKind::Struct) {
                        llvm::MDNode *metadata = llvm::MDNode::get(_context, llvm::MDString::get(_context, fds->GetRetType().GetVal()));
                        fun->setMetadata("struct_name", metadata);
                    }
                    llvm::MDNode *metadata = llvm::MDNode::get(_context, llvm::MDString::get(_context, is->GetStructName()));
                    fun->setMetadata("this_struct_name", metadata);
                    _functions.emplace(fun->getName(), fun);
                    _funArgsTypes.emplace(fun->getName().str(), argsAST);
                }
            }
            else if (StructStmt *ss = llvm::dyn_cast<StructStmt>(stmt)) {
                std::vector<llvm::Type *> fieldsTypes(ss->GetBody().size());
                std::unordered_map<std::string, Field> fields(ss->GetBody().size());
                for (int i = 0; i < fieldsTypes.size(); ++i) {
                    VarDeclStmt *vds = llvm::dyn_cast<VarDeclStmt>(ss->GetBody()[i]);
                    fieldsTypes[i] = typeToLLVM(vds->GetType());
                    fields.emplace(vds->GetName(), Field { vds->GetName(), typeToLLVM(vds->GetType()), vds->GetType(), vds->GetExpr() ? Visit(vds->GetExpr()) : nullptr,
                                   false, i });
                }
                llvm::StructType *structType = llvm::StructType::create(_context, fieldsTypes, ss->GetName());
                Struct s { .Name = ss->GetName(), .Type = structType, .Fields = fields, .TraitsImplements = {} };
                _structs.emplace(ss->GetName().str(), s);
            }
            else if (TraitDeclStmt *tds = llvm::dyn_cast<TraitDeclStmt>(stmt)) {
                Trait t { .Name = tds->GetName(), .Methods = {} };
                for (auto method : tds->GetBody()) {
                    if (FunDeclStmt *fds = llvm::dyn_cast<FunDeclStmt>(method)) {
                        std::vector<llvm::Type *> args(fds->GetArgs().size());
                        for (int i = 0; i < args.size(); ++i) {
                            args[i] = typeToLLVM(fds->GetArgs()[i].GetType());
                        }
                        t.Methods.push_back({ fds->GetName().str(), Method { .Name = fds->GetName().str(), .RetType = typeToLLVM(fds->GetRetType()), .Args = args } });
                    }
                }
                _traits.emplace(tds->GetName().str(), t);
            }
        }
    }

    llvm::Value *
    CodeGen::VisitVarDeclStmt(VarDeclStmt *vds) {
        llvm::Type *type = nullptr;
        llvm::Value *initializer = nullptr;
        std::string structName;
        bool isTraitType = vds->GetType().GetTypeKind() == ASTTypeKind::Trait;
        if (vds->GetExpr()) {
            initializer = Visit(vds->GetExpr());
            if (isTraitType) {
                type = initializer->getType();
                structName = resolveStructName(vds->GetExpr());
            }
            else {
                type = typeToLLVM(vds->GetType());
                initializer = implicitlyCast(initializer, typeToLLVM(vds->GetType()));
            }
        }
        else { 
            type = typeToLLVM(vds->GetType());
            if (vds->GetType().GetTypeKind() != ASTTypeKind::Struct) {
                initializer = llvm::Constant::getNullValue(typeToLLVM(vds->GetType()));
            }
            else {
                initializer = defaultStructConst(vds->GetType());
            }
        }
        llvm::Value *var;
        if (_vars.size() == 1) {
            var = new llvm::GlobalVariable(*_module, type, vds->IsConst(), llvm::GlobalValue::ExternalLinkage, llvm::cast<llvm::Constant>(initializer),
                                           vds->GetName());
        }
        else {
            var = _builder.CreateAlloca(type, nullptr, vds->GetName());
            _builder.CreateStore(initializer, var);
        }
        if (vds->GetType().GetTypeKind() == ASTTypeKind::Struct ||
            vds->GetType().GetTypeKind() == ASTTypeKind::Trait) {
            llvm::MDNode *metadata = llvm::MDNode::get(_context, llvm::MDString::get(_context, isTraitType ? structName : vds->GetType().GetVal()));
            if (_vars.size() == 1) {
                llvm::cast<llvm::GlobalVariable>(var)->setMetadata("struct_name", metadata);
            }
            else {
                llvm::cast<llvm::AllocaInst>(var)->setMetadata("struct_name", metadata);
            }
        }
        _vars.top().emplace(vds->GetName().str(), std::pair<llvm::Value *, llvm::Type *> { var, type });
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitVarAsgnStmt(VarAsgnStmt *vas) {
        llvm::Value *val = Visit(vas->GetExpr());
        auto varsCopy = _vars;
        while (!varsCopy.empty()) {
            if (auto var = varsCopy.top().find(vas->GetName().str()); var != varsCopy.top().end()) {
                val = implicitlyCast(val, var->second.second);
                if (auto glob = llvm::dyn_cast<llvm::GlobalVariable>(var->second.first)) {
                    _builder.CreateStore(val, glob);
                    return nullptr;
                }
                if (auto local = llvm::dyn_cast<llvm::AllocaInst>(var->second.first)) {
                    _builder.CreateStore(val, local);
                    return nullptr;
                }
                if (auto arg = llvm::dyn_cast<llvm::Argument>(var->second.first)) {
                    if (arg->getType()->isPointerTy()) {
                        _builder.CreateStore(val, arg);
                        return nullptr;
                    }
                    llvm::AllocaInst *alloca = _builder.CreateAlloca(arg->getType(), nullptr, arg->getName());
                    _builder.CreateStore(val, alloca);
                    return nullptr;
                }
            }
            varsCopy.pop();
        }
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitFunDeclStmt(FunDeclStmt *fds) {
        llvm::Function *fun = _functions.at(fds->GetName().str());
        llvm::BasicBlock *entry = llvm::BasicBlock::Create(_context, "entry", fun);
        _builder.SetInsertPoint(entry);
        _vars.push({});
        _funRetsTypes.push(fun->getReturnType());
        int index = 0;
        for (auto &arg : fun->args()) {
            arg.setName(fds->GetArgs()[index].GetName());
            _vars.top().emplace(arg.getName().str(), std::pair<llvm::Value *, llvm::Type *> { &arg, arg.getType()});
            ++index;
        }
        for (auto &stmt : fds->GetBody()) {
            Visit(stmt);
        }
        if (fds->GetRetType().GetTypeKind() == ASTTypeKind::Noth) {
            _builder.CreateRetVoid();
        }
        _funRetsTypes.pop();
        _vars.pop();
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitFunCallStmt(FunCallStmt *fcs) {
        FunCallExpr *expr = new FunCallExpr(fcs->GetName(), fcs->GetArgs(), fcs->GetStartLoc(), fcs->GetEndLoc());
        VisitFunCallExpr(expr);
        delete expr;
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitRetStmt(RetStmt *rs) {
        if (rs->GetExpr()) {
            llvm::Value *val = Visit(rs->GetExpr());
            return _builder.CreateRet(implicitlyCast(val, _funRetsTypes.top()));
        }
        return _builder.CreateRetVoid();
    }

    llvm::Value *
    CodeGen::VisitIfElseStmt(IfElseStmt *ies) {
        llvm::Function *parent = _builder.GetInsertBlock()->getParent();
        llvm::Value *cond = Visit(ies->GetCondition());
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(_context, "then", parent);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(_context, "else", parent);
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(_context, "merge", parent);
        _builder.CreateCondBr(cond, thenBB, elseBB);

        _builder.SetInsertPoint(thenBB);
        _vars.push({});
        for (auto &stmt : ies->GetThenBody()) {
            Visit(stmt);
        }
        _vars.pop();
        if (!_builder.GetInsertBlock()->getTerminator()) {
            _builder.CreateBr(mergeBB);
        }

        _builder.SetInsertPoint(elseBB);
        _vars.push({});
        for (auto &stmt : ies->GetElseBody()) {
            Visit(stmt);
        }
        _vars.pop();
        if (!_builder.GetInsertBlock()->getTerminator()) {
            _builder.CreateBr(mergeBB);
        }

        _builder.SetInsertPoint(mergeBB);
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitForLoopStmt(ForLoopStmt *fls) {
        llvm::Function *parent = _builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *indexatorBB = llvm::BasicBlock::Create(_context, "indexator", parent);
        llvm::BasicBlock *condBB = llvm::BasicBlock::Create(_context, "cond", parent);
        llvm::BasicBlock *iterationBB = llvm::BasicBlock::Create(_context, "iteration", parent);
        llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(_context, "body", parent);
        llvm::BasicBlock *exitBB = llvm::BasicBlock::Create(_context, "exit", parent);

        _builder.CreateBr(indexatorBB);
        _builder.SetInsertPoint(indexatorBB);
        if (fls->GetIndexator()) {
            Visit(fls->GetIndexator());
        }

        _builder.CreateBr(condBB);
        _builder.SetInsertPoint(condBB);
        llvm::Value *cond = Visit(fls->GetCondition());

        _builder.CreateCondBr(cond, bodyBB, exitBB);
        _builder.SetInsertPoint(bodyBB);
        _vars.push({});
        _loopDeth.push({ exitBB, iterationBB });
        for (auto &stmt : fls->GetBody()) {
            Visit(stmt);
        }
        _loopDeth.pop();
        _vars.pop();

        _builder.CreateBr(iterationBB);
        _builder.SetInsertPoint(iterationBB);
        if (fls->GetIteration()) {
            Visit(fls->GetIteration());
        }

        _builder.CreateBr(condBB);
        _builder.SetInsertPoint(exitBB);
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitBreakStmt(BreakStmt *bs) {
        _builder.CreateBr(_loopDeth.top().first);
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitContinueStmt(ContinueStmt *cs) {
        _builder.CreateBr(_loopDeth.top().second);
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitStructStmt(StructStmt *ss) {
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitFieldAsgnStmt(FieldAsgnStmt *fas) {
        createLoad = false;
        llvm::Value *obj = Visit(fas->GetObject());
        createLoad = true;
        Struct s = _structs.at(resolveStructName(fas->GetObject()));
        Field field = s.Fields.at(fas->GetName().str());
        llvm::Value *gep = _builder.CreateStructGEP(s.Type, obj, field.Index);
        llvm::Value *val = Visit(fas->GetExpr());
        val = implicitlyCast(val, field.Type);
        _builder.CreateStore(val, gep);
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitImplStmt(ImplStmt *is) {
        Struct s = _structs.at(is->GetStructName().str());
        for (auto &stmt : is->GetBody()) {
            FunDeclStmt *method = llvm::cast<FunDeclStmt>(stmt);
            llvm::Function *fun = _functions.at((s.Name + "." + method->GetName()).str());
            llvm::BasicBlock *entry = llvm::BasicBlock::Create(_context, "entry", fun);
            _builder.SetInsertPoint(entry);
            _vars.push({});
            _funRetsTypes.push(fun->getReturnType());
            int index = 0;
            for (auto &arg : fun->args()) {
                arg.setName(index == 0 ? "this" : method->GetArgs()[index - 1].GetName());
                _vars.top().emplace(arg.getName().str(), std::pair<llvm::Value *, llvm::Type *> { &arg, arg.getType() });
                ++index;
            }
            for (auto &stmt : method->GetBody()) {
                Visit(stmt);
            }
            if (method->GetRetType().GetTypeKind() == ASTTypeKind::Noth) {
                _builder.CreateRetVoid();
            }
            _funRetsTypes.pop();
            _vars.pop();
        }
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitMethodCallStmt(MethodCallStmt *mcs) {
        MethodCallExpr *expr = new MethodCallExpr(mcs->GetObject(), mcs->GetName(), mcs->GetArgs(), mcs->GetStartLoc(), mcs->GetEndLoc());
        VisitMethodCallExpr(expr);
        delete expr;
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitTraitDeclStmt(TraitDeclStmt *tds) {
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitEchoStmt(EchoStmt *es) {
        std::string format;
        llvm::Value *val = Visit(es->GetRHS());
        llvm::Type *type = val->getType();
        if (type->isIntegerTy()) {
            format = "%ld";
        }
        else if (type->isFloatingPointTy()) {
            format = "%g";
            if (type->isFloatTy()) {
                val = _builder.CreateFPExt(val, llvm::Type::getDoubleTy(_context));
            }
        }
        else if (type->isPointerTy()) {
            format = "%p";
        }
        format += "\n";
        _builder.CreateCall(_module->getFunction("printf"), { _builder.CreateGlobalString(format, "printf.format"), val });
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitBinaryExpr(BinaryExpr *be) {
        llvm::Value *lhs = Visit(be->GetLHS());
        llvm::Value *rhs = Visit(be->GetRHS());
        llvm::Type *lhsType = lhs->getType();
        llvm::Type *rhsType = rhs->getType();
        llvm::Type *commonType = getCommonType(lhsType, rhsType);
        if (lhsType != commonType) {
            lhs = implicitlyCast(lhs, commonType);
            lhsType = lhs->getType();
        }
        if (rhsType != commonType) {
            rhs = implicitlyCast(rhs, commonType);
            rhsType = rhs->getType();
        }
        switch (be->GetOp().GetKind()) {
            case TkPlus:
                if (commonType->isFloatingPointTy()) {
                    return _builder.CreateFAdd(lhs, rhs);
                }
                return _builder.CreateAdd(lhs, rhs);
            case TkMinus:
                if (commonType->isFloatingPointTy()) {
                    return _builder.CreateFSub(lhs, rhs);
                }
                return _builder.CreateSub(lhs, rhs);
            case TkStar:
                if (commonType->isFloatingPointTy()) {
                    return _builder.CreateFMul(lhs, rhs);
                }
                return _builder.CreateMul(lhs, rhs);
            case TkSlash:
                if (commonType->isFloatingPointTy()) {
                    return _builder.CreateFDiv(lhs, rhs);
                }
                return _builder.CreateSDiv(lhs, rhs);
            case TkPercent:
                if (commonType->isFloatingPointTy()) {
                    return _builder.CreateFRem(lhs, rhs);
                }
                return _builder.CreateSRem(lhs, rhs);
            case TkLogAnd:
                return _builder.CreateLogicalAnd(lhs, rhs);
            case TkLogOr:
                return _builder.CreateLogicalOr(lhs, rhs);
            case TkAnd:
                return _builder.CreateAnd(lhs, rhs);
            case TkOr:
                return _builder.CreateOr(lhs, rhs);
            case TkGt:
                if (commonType->isFloatingPointTy()) {
                    return _builder.CreateFCmpOGT(lhs, rhs);
                }
                return _builder.CreateICmpSGT(lhs, rhs);
            case TkGtEq:
                if (commonType->isFloatingPointTy()) {
                    return _builder.CreateFCmpOGE(lhs, rhs);
                }
                return _builder.CreateICmpSGE(lhs, rhs);
            case TkLt:
                if (commonType->isFloatingPointTy()) {
                    return _builder.CreateFCmpOLT(lhs, rhs);
                }
                return _builder.CreateICmpSLT(lhs, rhs);
            case TkLtEq:
                if (commonType->isFloatingPointTy()) {
                    return _builder.CreateFCmpOLE(lhs, rhs);
                }
                return _builder.CreateICmpSLE(lhs, rhs);
            case TkEqEq:
                if (commonType->isFloatingPointTy()) {
                    return _builder.CreateFCmpOEQ(lhs, rhs);
                }
                return _builder.CreateICmpEQ(lhs, rhs);
            case TkNotEq:
                if (commonType->isFloatingPointTy()) {
                    return _builder.CreateFCmpONE(lhs, rhs);
                }
                return _builder.CreateICmpNE(lhs, rhs);
            default: {}
        }
        return nullptr;
    }
    
    llvm::Value *
    CodeGen::VisitUnaryExpr(UnaryExpr *ue) {
        llvm::Value *rhs = Visit(ue->GetRHS());
        switch (ue->GetOp().GetKind()) {
            case TkMinus:
                if (rhs->getType()->isFloatingPointTy()) {
                    return _builder.CreateFNeg(rhs);
                }
                return _builder.CreateNeg(rhs);
            case TkBang:
                return _builder.CreateNot(rhs);
            default: {}
        }
        return nullptr;
    }
    
    llvm::Value *
    CodeGen::VisitVarExpr(VarExpr *ve) {
        auto varsCopy = _vars;
        while (!varsCopy.empty()) {
            if (auto var = varsCopy.top().find(ve->GetName().str()); var != varsCopy.top().end()) {
                if (createLoad) {
                    if (auto glob = llvm::dyn_cast<llvm::GlobalVariable>(var->second.first)) {
                        if (_vars.size() == 1) {
                            return glob->getInitializer();
                        }
                        return _builder.CreateLoad(glob->getValueType(), glob, var->first + ".load");
                    }
                    if (auto local = llvm::dyn_cast<llvm::AllocaInst>(var->second.first)) {
                        return _builder.CreateLoad(local->getAllocatedType(), local, var->first + ".load");
                    }
                }
                return var->second.first;
            }
            varsCopy.pop();
        }
        return nullptr;
    }
    
    llvm::Value *
    CodeGen::VisitLiteralExpr(LiteralExpr *le) {
        switch (le->GetVal().GetType().GetTypeKind()) {
            #define CONST_INT(func, field) llvm::ConstantInt::get(llvm::Type::func(_context), le->GetVal().GetData().field)
            #define CONST_FP(func, field) llvm::ConstantFP::get(llvm::Type::func(_context), le->GetVal().GetData().field)
            case ASTTypeKind::Bool:
                return CONST_INT(getInt1Ty, boolVal);
            case ASTTypeKind::Char:
                return CONST_INT(getInt8Ty, charVal);
            case ASTTypeKind::I16:
                return CONST_INT(getInt16Ty, i16Val);
            case ASTTypeKind::I32:
                return CONST_INT(getInt32Ty, i32Val);
            case ASTTypeKind::I64:
                return CONST_INT(getInt64Ty, i64Val);
            case ASTTypeKind::F32:
                return CONST_FP(getFloatTy, f32Val);
            case ASTTypeKind::F64:
                return CONST_FP(getDoubleTy, f64Val);
            default: {}
            #undef CONST_FP
            #undef CONST_INT
        }
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitFunCallExpr(FunCallExpr *fce) {
        llvm::Function *fun = _functions.at(fce->GetName().str());
        std::vector<llvm::Value *> args(fce->GetArgs().size());
        for (int i = 0; i < fce->GetArgs().size(); ++i) {
            llvm::Value *val = Visit(fce->GetArgs()[i]);
            llvm::Type *expectedType = fun->getFunctionType()->getParamType(i);

            if (expectedType->isStructTy() && expectedType->getStructNumElements() == 2 && 
                !val->getType()->isPointerTy()) {
                
                std::string structName = resolveStructName(fce->GetArgs()[i]);
                std::string traitName = _funArgsTypes.at(fce->GetName().str())[i].GetVal().str();

                llvm::Value *fatPtr = llvm::UndefValue::get(expectedType);
                
                if (!val->getType()->isPointerTy()) {
                    llvm::AllocaInst *tmp = _builder.CreateAlloca(val->getType());
                    _builder.CreateStore(val, tmp);
                    val = tmp;
                }
                llvm::Value *dataPtr = _builder.CreateBitCast(val, llvm::PointerType::get(_context, 0));
                fatPtr = _builder.CreateInsertValue(fatPtr, dataPtr, 0);

                llvm::Value *vtable = getOrCreateVTable(structName, traitName);
                llvm::Value *vtablePtr = _builder.CreateBitCast(vtable, llvm::PointerType::get(llvm::PointerType::get(_context, 0), 0));
                fatPtr = _builder.CreateInsertValue(fatPtr, vtablePtr, 1);
                
                args[i] = fatPtr;
            }
            else {
                args[i] = implicitlyCast(val, expectedType);
            }
        }
        return _builder.CreateCall(fun, args, fun->getName() + ".call");
    }

    llvm::Value *
    CodeGen::VisitStructExpr(StructExpr *se) {
        Struct s = _structs.at(se->GetName().str());
        if (_vars.size() != 1) {
            llvm::AllocaInst *alloca = _builder.CreateAlloca(s.Type, nullptr, s.Name + ".alloca");
            for (int i = 0; i < se->GetInitializer().size(); ++i) {
                std::string name = se->GetInitializer()[i].first.str();
                llvm::Value *fieldPtr = _builder.CreateStructGEP(s.Type, alloca, s.Fields.at(name).Index, name + ".gep");
                llvm::Value *val = Visit(se->GetInitializer()[i].second);
                val = implicitlyCast(val, s.Fields.at(se->GetInitializer()[i].first.str()).Type);
                _builder.CreateStore(val, fieldPtr);
                s.Fields.at(name).ManualInitialized = true;
            }
            for (auto &field : s.Fields) {
                if (!field.second.ManualInitialized) {
                    llvm::Value *fieldPtr = _builder.CreateStructGEP(s.Type, alloca, field.second.Index, field.second.Name + ".gep");
                    llvm::Value *val;
                    if (field.second.ASTType.GetTypeKind() == ASTTypeKind::Struct) {
                        val = defaultStructConst(field.second.ASTType);
                    }
                    else {
                        val = llvm::Constant::getNullValue(field.second.Type);
                    }
                    _builder.CreateStore(val, fieldPtr);
                }
            }
            return _builder.CreateLoad(s.Type, alloca, s.Name + ".alloca.load");
        }
        else {
            std::vector<llvm::Constant *> fields;
            for (int i = 0; i < se->GetInitializer().size(); ++i) {
                std::string name = se->GetInitializer()[i].first.str();
                fields.push_back(llvm::dyn_cast<llvm::Constant>(Visit(se->GetInitializer()[i].second)));
                s.Fields.at(name).ManualInitialized = true;
            }
            for (auto &field : s.Fields) {
                if (!field.second.ManualInitialized) {
                    if (field.second.ASTType.GetTypeKind() == ASTTypeKind::Struct) {
                        fields.push_back(llvm::dyn_cast<llvm::Constant>(defaultStructConst(field.second.ASTType)));
                    }
                    else {
                        fields.push_back(llvm::Constant::getNullValue(field.second.Type));
                    }
                }
            }
            return llvm::ConstantStruct::get(s.Type, fields);
        }
    }

    llvm::Value *
    CodeGen::VisitFieldAccessExpr(FieldAccessExpr *fae) {
        createLoad = false;
        llvm::Value *obj = Visit(fae->GetObject());
        createLoad = true;
        if (!obj->getType()->isPointerTy()) {
            llvm::AllocaInst *tempAlloca = _builder.CreateAlloca(obj->getType(), nullptr, "rvalue.tmp");
            _builder.CreateStore(obj, tempAlloca);
            obj = tempAlloca;
        }
        Struct s = _structs.at(resolveStructName(fae->GetObject()));
        Field field = s.Fields.at(fae->GetName().str());
        llvm::Value *gep = _builder.CreateStructGEP(s.Type, obj, field.Index);
        if (_vars.size() == 1) {
            if (auto *globalVar = llvm::dyn_cast<llvm::GlobalVariable>(obj)) {
                if (globalVar->hasInitializer()) {
                    obj = globalVar->getInitializer();
                }
                else {
                    return nullptr; 
                }
            }
            if (auto *constantVal = llvm::dyn_cast<llvm::Constant>(obj)) {
                return constantVal->getAggregateElement(field.Index);
            }
            return nullptr;
        }
        return _builder.CreateLoad(s.Type->getTypeAtIndex(field.Index), gep, fae->GetName() + ".load");
    }

    llvm::Value *
    CodeGen::VisitMethodCallExpr(MethodCallExpr *mce) {
        createLoad = false;
        llvm::Value *obj = Visit(mce->GetObject());
        createLoad = true;

        std::string structName = resolveStructName(mce->GetObject());
        if (structName.empty() && obj->getType()->isStructTy()) {
            structName = obj->getType()->getStructName().str();
            size_t dotPos = structName.find('.');
            if (dotPos != std::string::npos) {
                structName = structName.substr(0, dotPos);
            }
        }

        if (_traits.count(structName)) {
            llvm::Value *thisPtr = _builder.CreateExtractValue(obj, 0, "trait.this");
            llvm::Value *vtablePtr = _builder.CreateExtractValue(obj, 1, "trait.vtable");

            Trait &t = _traits.at(structName);
            int methodIndex = -1;
            llvm::Type* retType = nullptr;

            for (int i = 0; i < t.Methods.size(); ++i) {
                if (t.Methods[i].first == mce->GetName().str()) {
                    methodIndex = i;
                    retType = t.Methods[i].second.RetType;
                    break;
                }
            }

            if (methodIndex == -1) {
                return nullptr;
            }

            llvm::Type *voidPtrTy = llvm::PointerType::get(_context, 0);
            llvm::Value *gep = _builder.CreateConstGEP1_32(voidPtrTy, vtablePtr, methodIndex);
            llvm::Value *funRaw = _builder.CreateLoad(voidPtrTy, gep, "vfunc.ptr");

            std::vector<llvm::Value *> args(mce->GetArgs().size() + 1);
            args[0] = thisPtr;
            for (int i = 0; i < mce->GetArgs().size(); ++i) {
                args[i + 1] = implicitlyCast(Visit(mce->GetArgs()[i]), t.Methods[methodIndex].second.Args[i]);
            }
            llvm::FunctionType *method = llvm::FunctionType::get(retType, t.Methods[methodIndex].second.Args, false);

            return _builder.CreateCall(method, funRaw, args);
        }

        std::string fullName = structName + "." + mce->GetName().str();
        if (_functions.find(fullName) == _functions.end()) {
            return nullptr;
        }

        llvm::Function *fun = _functions.at(fullName);
        std::vector<llvm::Value *> args(fun->getFunctionType()->getNumParams());

        if (!obj->getType()->isPointerTy()) {
            llvm::AllocaInst *alloca = _builder.CreateAlloca(obj->getType());
            _builder.CreateStore(obj, alloca);
            obj = alloca;
        }
        args[0] = obj;

        for (int i = 0; i < mce->GetArgs().size(); ++i) {
            llvm::Value *val = Visit(mce->GetArgs()[i]);
            llvm::Type *expectedType = fun->getFunctionType()->getParamType(i + 1);

            if (expectedType->isStructTy() && expectedType->getStructNumElements() == 2 && 
                !val->getType()->isPointerTy()) {
                
                std::string structName = resolveStructName(mce->GetArgs()[i]);
                std::string traitName = _funArgsTypes.at(fullName)[i + 1].GetVal().str();

                llvm::Value *fatPtr = llvm::UndefValue::get(expectedType);
                
                if (!val->getType()->isPointerTy()) {
                    llvm::AllocaInst *tmp = _builder.CreateAlloca(val->getType());
                    _builder.CreateStore(val, tmp);
                    val = tmp;
                }
                llvm::Value *dataPtr = _builder.CreateBitCast(val, llvm::PointerType::get(_context, 0));
                fatPtr = _builder.CreateInsertValue(fatPtr, dataPtr, 0);

                llvm::Value *vtable = getOrCreateVTable(structName, traitName);
                llvm::Value *vtablePtr = _builder.CreateBitCast(vtable, llvm::PointerType::get(llvm::PointerType::get(_context, 0), 0));
                fatPtr = _builder.CreateInsertValue(fatPtr, vtablePtr, 1);
                
                args[i + 1] = fatPtr;
            }
            else {
                args[i + 1] = implicitlyCast(val, expectedType);
            }
        }
        return _builder.CreateCall(fun, args);
    }
    
    llvm::Type *
    CodeGen::getCommonType(llvm::Type *left, llvm::Type *right) {
        if (left == right) {
            return left;
        }
        else if (left->isIntegerTy() || right->isIntegerTy()) {
            if (left->isIntegerTy() && right->isIntegerTy()) {
                unsigned leftWidth = left->getIntegerBitWidth();
                unsigned rightWidth = right->getIntegerBitWidth();

                return leftWidth > rightWidth ? left : right;
            }
            else if (left->isFloatingPointTy() || right->isFloatingPointTy()) {
                return left->isFloatingPointTy() ? left : right;
            }
        }
        else if (left->isFloatingPointTy() && right->isFloatingPointTy()) {
            return left->isDoubleTy() && right->isFloatTy() ? left : right;
        }
        else if (left->isDoubleTy() || right->isDoubleTy()) {
            return llvm::Type::getDoubleTy(_context);
        }
        return nullptr;
    }

    llvm::Value *
    CodeGen::implicitlyCast(llvm::Value *src, llvm::Type *expectType) {
        llvm::Type *destType = src->getType();
        if (destType == expectType) {
            return src;
        }
        if (expectType->isStructTy() && expectType->getStructNumElements() == 2) {
            if (src->getType() == expectType) {
                return src;
            }

            std::string traitName = expectType->getStructName().str();
            size_t dotPos = traitName.find('.');
            if (dotPos != std::string::npos) {
                traitName = traitName.substr(0, dotPos);
            }

            if (_traits.count(traitName)) {
                llvm::Value* fatPtr = llvm::UndefValue::get(expectType);

                llvm::Value *dataPtr;
                if (src->getType()->isPointerTy()) {
                    dataPtr = _builder.CreateBitCast(src, llvm::PointerType::get(_context, 0));
                }
                else {
                    llvm::AllocaInst *tmp = _builder.CreateAlloca(src->getType());
                    _builder.CreateStore(src, tmp);
                    dataPtr = _builder.CreateBitCast(tmp, llvm::PointerType::get(_context, 0));
                }
                fatPtr = _builder.CreateInsertValue(fatPtr, dataPtr, 0);
                llvm::Value *vtable = getOrCreateVTable(src->getType()->getStructName().str(), traitName);
                llvm::Value *vtablePtr = _builder.CreateBitCast(vtable, llvm::PointerType::get(llvm::PointerType::get(_context, 0), 0));
                fatPtr = _builder.CreateInsertValue(fatPtr, vtablePtr, 1);
                return fatPtr;
            }
        }
        else if (destType->isIntegerTy() && expectType->isIntegerTy()) {
            unsigned long valWidth = destType->getIntegerBitWidth();
            unsigned long expectedWidth = expectType->getIntegerBitWidth();

            if (valWidth > expectedWidth) {
                return _builder.CreateTrunc(src, expectType, "trunc.tmp");
            }
            else {
                return _builder.CreateSExt(src, expectType, "sext.tmp");
            }
        }
        else if (destType->isFloatingPointTy() && expectType->isFloatingPointTy()) {
            if (destType->isFloatTy() && expectType->isDoubleTy()) {
                return _builder.CreateFPExt(src, expectType, "fpext.tmp");
            }
            else {
                return _builder.CreateFPTrunc(src, expectType, "fptrunc.tmp");
            }
        }
        else if (destType->isIntegerTy() && expectType->isFloatingPointTy()) {
            return _builder.CreateSIToFP(src, expectType, "sitofp.tmp");
        }
        return src;
    }

    llvm::SMRange
    CodeGen::getRange(llvm::SMLoc start, int len) const {
        return llvm::SMRange(start, llvm::SMLoc::getFromPointer(start.getPointer() + len));
    }

    llvm::Type *
    CodeGen::typeToLLVM(ASTType type) {
        switch (type.GetTypeKind()) {
            #define TYPE(func) llvm::Type::func(_context);
            case ASTTypeKind::Bool:
                return TYPE(getInt1Ty);
            case ASTTypeKind::Char:
                return TYPE(getInt8Ty);
            case ASTTypeKind::I16:
                return TYPE(getInt16Ty);
            case ASTTypeKind::I32:
                return TYPE(getInt32Ty);
            case ASTTypeKind::I64:
                return TYPE(getInt64Ty);
            case ASTTypeKind::F32:
                return TYPE(getFloatTy);
            case ASTTypeKind::F64:
                return TYPE(getDoubleTy);
            case ASTTypeKind::Struct:
                return _structs.at(type.GetVal().str()).Type;
            case ASTTypeKind::Trait: {
                llvm::StructType *existingType = llvm::StructType::getTypeByName(_context, type.GetVal().str());
                if (existingType) {
                    return existingType;
                }
                llvm::Type *voidPtrTy = llvm::PointerType::get(_context, 0);
                llvm::Type *vtablePtrTy = llvm::PointerType::get(voidPtrTy, 0);
                return llvm::StructType::create(_context, { voidPtrTy, vtablePtrTy }, type.GetVal());
            }
            case ASTTypeKind::Noth:
                return TYPE(getVoidTy);
            #undef TYPE
        }
    }

    llvm::Value *
    CodeGen::defaultStructConst(ASTType type) {
        Struct s = _structs.at(type.GetVal().str());
        std::vector<llvm::Constant *> fields(s.Fields.size());
        int i = 0;
        for (auto &field : s.Fields) {
            if (field.second.Val) {
                fields[i] = llvm::cast<llvm::Constant>(field.second.Val);
            }
            else {
                if (field.second.ASTType.GetTypeKind() == ASTTypeKind::Struct) {
                    fields[i] = llvm::dyn_cast<llvm::Constant>(defaultStructConst(field.second.ASTType));
                }
                else {
                    fields[i] = llvm::Constant::getNullValue(field.second.Type);
                }
            }
        }
        return llvm::ConstantStruct::get(s.Type, fields);
    }

    
    std::string
    CodeGen::resolveStructName(Expr *expr) {
        switch (expr->GetKind()) {
            case NkVarExpr: {
                std::string name = llvm::dyn_cast<VarExpr>(expr)->GetName().str();
                auto varsCopy = _vars;
                while (!varsCopy.empty()) {
                    for (auto var : varsCopy.top()) {
                        if (var.first == name) {
                            if (auto *glob = llvm::dyn_cast<llvm::GlobalVariable>(var.second.first)) {
                                if (auto *metadata = glob->getMetadata("struct_name")) {
                                    if (auto *mdStr = llvm::dyn_cast<llvm::MDString>(metadata->getOperand(0))) {
                                        return mdStr->getString().str();
                                    }
                                }
                            } 
                            else if (auto *local = llvm::dyn_cast<llvm::AllocaInst>(var.second.first)) {
                                if (auto *metadata = local->getMetadata("struct_name")) {
                                    if (auto *mdStr = llvm::dyn_cast<llvm::MDString>(metadata->getOperand(0))) {
                                        return mdStr->getString().str();
                                    }
                                }
                            }
                            else if (auto *arg = llvm::dyn_cast<llvm::Argument>(var.second.first)) {
                                llvm::Function *parent = arg->getParent();
                                if (parent->getFunctionType()->getParamType(0) == arg->getType()) {
                                    if (auto *metadata = parent->getMetadata("this_struct_name")) {
                                        if (auto *mdStr = llvm::dyn_cast<llvm::MDString>(metadata->getOperand(0))) {
                                            return mdStr->getString().str();
                                        }
                                    }
                                }
                                else {
                                    if (arg->getType()->isStructTy()) {
                                        return arg->getType()->getStructName().str();
                                    }
                                }
                            }
                        }
                    }
                    varsCopy.pop();
                }
                return "";
            }
            case NkFunCallExpr: {
                std::string name = llvm::dyn_cast<FunCallExpr>(expr)->GetName().str();
                llvm::Function *fun = _functions.at(name);
                if (auto *metadata = fun->getMetadata("struct_name")) {
                    if (auto *mdStr = llvm::dyn_cast<llvm::MDString>(metadata->getOperand(0))) {
                        return mdStr->getString().str();
                    }
                }
                return "";
            }
            case NkFieldAccessExpr: {
                FieldAccessExpr *fae = llvm::dyn_cast<FieldAccessExpr>(expr);
                std::string parentStructName = resolveStructName(fae->GetObject());
                if (parentStructName.empty()) {
                    return "";
                }

                if (_structs.count(parentStructName)) {
                    Struct &s = _structs.at(parentStructName);
                    std::string fieldName = fae->GetName().str();
                    
                    if (s.Fields.count(fieldName)) {
                        Field &f = s.Fields.at(fieldName);
                        if (f.ASTType.GetTypeKind() == ASTTypeKind::Struct) {
                            return f.ASTType.GetVal().str();
                        }
                    }
                }
                return "";
            }
            case NkMethodCallExpr: {
                MethodCallExpr *mce = llvm::dyn_cast<MethodCallExpr>(expr);
                std::string parentStructName = resolveStructName(mce->GetObject());
                if (parentStructName.empty()) {
                    return "";
                }

                llvm::Function *fun = _functions.at(parentStructName + "." + mce->GetName().str());
                if (auto *metadata = fun->getMetadata("struct_name")) {
                    if (auto *mdStr = llvm::dyn_cast<llvm::MDString>(metadata->getOperand(0))) {
                        return mdStr->getString().str();
                    }
                }
                return "";
            }
            default: {
                return "";
            }
        }
    }

    llvm::Value *
    CodeGen::getOrCreateVTable(const std::string &structName, const std::string &traitName) {
        std::string vtableName = "vtable." + structName + ".as." + traitName;
        if (auto *existingVTable = _module->getNamedGlobal(vtableName)) {
            return existingVTable;
        }

        Trait &t = _traits.at(traitName);
        std::vector<llvm::Constant *> functions(t.Methods.size());
        llvm::Type *voidPtrTy = llvm::PointerType::get(_context, 0);

        int i = 0;
        for (const auto &[methodName, _] : t.Methods) {
            std::string fullMethodName = structName + "." + methodName;
            llvm::Function *fun = _functions.at(fullMethodName);
            functions[i] = llvm::ConstantExpr::getBitCast(fun, voidPtrTy);
            ++i;
        }

        llvm::ArrayType *vtableArrTy = llvm::ArrayType::get(voidPtrTy, functions.size());
        llvm::GlobalVariable *vtable = new llvm::GlobalVariable(*_module, vtableArrTy, true, llvm::GlobalValue::InternalLinkage,
                                                                  llvm::ConstantArray::get(vtableArrTy, functions), vtableName);
        return vtable;
    }
}
