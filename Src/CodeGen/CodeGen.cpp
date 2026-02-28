#include <iostream>
#include <marble/Basic/ModuleManager.h>
#include <marble/CodeGen/CodeGen.h>

static bool createLoad = true;

static std::vector<std::string>
splitString(std::string src, char separator);

namespace marble {
    void
    CodeGen::DeclareMod(Module *mod) {
        if (!mod) {
            return;
        }

        bool inRoot = false;
        if (!_curMod) {
            declareRuntimeFunctions();
            inRoot = true;
        }

        Module *oldMod = _curMod ? _curMod : mod;
        _curMod = mod;

        for (auto &stmt : mod->AST) {
            if (VarDeclStmt *vds = llvm::dyn_cast<VarDeclStmt>(stmt)) {
                VisitVarDeclStmt(vds);
            }
            else if (FunDeclStmt *fds = llvm::dyn_cast<FunDeclStmt>(stmt)) {
                std::vector<llvm::Type *> args(fds->GetArgs().size());
                std::vector<ASTType> argsAST(fds->GetArgs().size());
                for (int i = 0; i < fds->GetArgs().size(); ++i) {
                    args[i] = typeToLLVM(fds->GetArgs()[i].GetType());
                    argsAST[i] = fds->GetArgs()[i].GetType();
                }
                llvm::FunctionType *retType = llvm::FunctionType::get(typeToLLVM(fds->GetRetType()), args, false);
                llvm::Function *fun = llvm::Function::Create(retType, llvm::GlobalValue::ExternalLinkage, fds->GetName() == "main" ? "main"
                                                                                                                                   : getMangledName(fds->GetName()), *_module);
                
                if (fds->GetRetType().GetTypeKind() == ASTTypeKind::Struct) {
                    llvm::MDNode *metadata = llvm::MDNode::get(_context, llvm::MDString::get(_context, getMangledName(fds->GetRetType())));
                    fun->setMetadata("struct_name", metadata);
                }
                _functions.emplace(fun->getName(), fun);
                _funArgsTypes.emplace(fun->getName().str(), argsAST);
            }
            else if (ImplStmt *is = llvm::dyn_cast<ImplStmt>(stmt)) {
                Struct &s = _structs.at(getMangledName(is->GetStructType()));
                if (is->GetTraitType() != ASTType()) {
                    s.TraitsImplements.emplace(getMangledName(is->GetTraitType()), _traits.at(getMangledName(is->GetTraitType())));
                }
                for (auto stmt : is->GetBody()) {
                    FunDeclStmt *fds = llvm::cast<FunDeclStmt>(stmt);
                    std::vector<llvm::Type *> args(fds->GetArgs().size() + 1);
                    std::vector<ASTType> argsAST(fds->GetArgs().size() + 1);
                    args[0] = llvm::PointerType::get(_structs.at(getMangledName(is->GetStructType())).Type, 0);
                    argsAST[0] = is->GetStructType();
                    for (int i = 0; i < fds->GetArgs().size(); ++i) {
                        args[i + 1] = typeToLLVM(fds->GetArgs()[i].GetType());
                        argsAST[i + 1] = fds->GetArgs()[i].GetType();
                    }
                    llvm::FunctionType *retType = llvm::FunctionType::get(typeToLLVM(fds->GetRetType()), args, false);
                    llvm::Function *fun = llvm::Function::Create(retType, llvm::GlobalValue::ExternalLinkage, getMangledName(is->GetStructType()) + "." + fds->GetName(),
                                                                 *_module);
                    
                    if (fds->GetRetType().GetTypeKind() == ASTTypeKind::Struct) {
                        llvm::MDNode *metadata = llvm::MDNode::get(_context, llvm::MDString::get(_context, getMangledName(fds->GetRetType())));
                        fun->setMetadata("struct_name", metadata);
                    }
                    llvm::MDNode *metadata = llvm::MDNode::get(_context, llvm::MDString::get(_context, getMangledName(is->GetStructType())));
                    fun->setMetadata("this_struct_name", metadata);
                    _functions.emplace(fun->getName(), fun);
                    _funArgsTypes.emplace(fun->getName().str(), argsAST);
                }
            }
            else if (StructStmt *ss = llvm::dyn_cast<StructStmt>(stmt)) {
                std::vector<llvm::Type *> fieldsTypes(ss->GetBody().size());
                std::unordered_map<std::string, Field> fields;
                llvm::StructType *structType = llvm::StructType::create(_context, getMangledName(ss->GetName()));
                Struct s { .Name = ss->GetName(), .MangledName = structType->getName().str(), .Type = structType, .Fields = fields, .TraitsImplements = {} };
                _structs.emplace(s.MangledName, s);
                for (int i = 0; i < fieldsTypes.size(); ++i) {
                    VarDeclStmt *vds = llvm::dyn_cast<VarDeclStmt>(ss->GetBody()[i]);
                    fieldsTypes[i] = typeToLLVM(vds->GetType());
                    fields.emplace(vds->GetName(), Field { .Name = vds->GetName(), .Type = typeToLLVM(vds->GetType()), .ASTType = vds->GetType(),
                                                           .DefaultExpr = vds->GetExpr(), .ManualInitialized = false, .Index = i });

                    _structs.at(s.MangledName).Fields.emplace(vds->GetName(), Field { .Name = vds->GetName(), .Type = fieldsTypes[i], .ASTType = vds->GetType(),
                                                                                      .DefaultExpr = vds->GetExpr(), .ManualInitialized = false, .Index = i });
                }
                structType->setBody(fieldsTypes);
            }
            else if (TraitDeclStmt *tds = llvm::dyn_cast<TraitDeclStmt>(stmt)) {
                Trait t { .Name = tds->GetName(), .MangledName = getMangledName(tds->GetName()), .Methods = {} };
                for (auto method : tds->GetBody()) {
                    if (FunDeclStmt *fds = llvm::dyn_cast<FunDeclStmt>(method)) {
                        std::vector<llvm::Type *> args(fds->GetArgs().size());
                        for (int i = 0; i < args.size(); ++i) {
                            args[i] = typeToLLVM(fds->GetArgs()[i].GetType());
                        }
                        t.Methods.push_back({ fds->GetName(), Method { .Name = fds->GetName(), .RetType = typeToLLVM(fds->GetRetType()), .Args = args } });
                    }
                }
                _traits.emplace(t.MangledName, t);
            }
            else if (ImportStmt *is = llvm::dyn_cast<ImportStmt>(stmt)) {
                std::string path = is->IsLocalImport() ? _parentDir + "/" + is->GetPath() : ModuleManager::LibsPath + is->GetPath();
                Module *import = ModuleManager::LoadModule(path, _srcMgr, _diag);
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
                DeclareMod(import);
            }
            else if (ModDeclStmt *mds = llvm::dyn_cast<ModDeclStmt>(stmt)) {
                Module *sub = mod->Submodules.at(mds->GetName());
                DeclareMod(mod->Submodules.at(mds->GetName()));
            }
        }

        if (!inRoot) {
            for (auto &stmt : _curMod->AST) {
                Visit(stmt);
            }
        }

        _curMod = oldMod;
    }

    llvm::Value *
    CodeGen::VisitVarDeclStmt(VarDeclStmt *vds) {
        llvm::Type *type = typeToLLVM(vds->GetType());
        llvm::Value *initializer = nullptr;
        bool isTraitType = vds->GetType().GetTypeKind() == ASTTypeKind::Trait;
        if (vds->GetExpr()) {
            initializer = Visit(vds->GetExpr());
            if (isTraitType) {
                std::string concreteStruct = resolveStructName(vds->GetExpr());
                initializer = castToTrait(initializer, type, concreteStruct);
            }
            else {
                type = typeToLLVM(vds->GetType());
                initializer = implicitlyCast(initializer, typeToLLVM(vds->GetType()));
            }
        }
        else { 
            if (isTraitType) {
                initializer = llvm::ConstantAggregateZero::get(llvm::cast<llvm::StructType>(type));
            }
            else if (vds->GetType().GetTypeKind() != ASTTypeKind::Struct) {
                initializer = llvm::Constant::getNullValue(typeToLLVM(vds->GetType()));
            }
            else {
                if (vds->GetType().IsPointer()) {
                    initializer = llvm::ConstantPointerNull::get(_builder.getPtrTy());
                }
                else {
                    initializer = defaultStructConst(vds->GetType());
                }
            }
        }
        llvm::Value *var;
        if (_vars.size() == 1) {
            var = new llvm::GlobalVariable(*_module, type, vds->IsConst(), llvm::GlobalValue::ExternalLinkage, llvm::cast<llvm::Constant>(initializer),
                                           getMangledName(vds->GetName()));
        }
        else {
            var = _builder.CreateAlloca(type, nullptr, getMangledName(vds->GetName()));
            _builder.CreateStore(initializer, var);
        }
        if (vds->GetType().GetTypeKind() == ASTTypeKind::Struct ||
            vds->GetType().GetTypeKind() == ASTTypeKind::Trait) {
            llvm::MDNode *metadata = llvm::MDNode::get(_context, llvm::MDString::get(_context, getMangledName(vds->GetType())));
            if (_vars.size() == 1) {
                llvm::cast<llvm::GlobalVariable>(var)->setMetadata("struct_name", metadata);
            }
            else {
                llvm::cast<llvm::AllocaInst>(var)->setMetadata("struct_name", metadata);
            }
        }
        std::string varName = _vars.size() == 1 ? getMangledName(vds->GetName()) : vds->GetName();
        _vars.top().emplace(varName, std::make_tuple(var, type, vds->GetType()));
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitVarAsgnStmt(VarAsgnStmt *vas) {
        llvm::Value *val = Visit(vas->GetExpr());
        std::string varName = _vars.size() == 1 ? getMangledName(vas->GetName()) : vas->GetName();
        auto varsCopy = _vars;
        while (!varsCopy.empty()) {
            if (auto var = varsCopy.top().find(varName); var != varsCopy.top().end()) {
                auto &[varVal, llvmType, type] = var->second;
                if (auto arg = llvm::dyn_cast<llvm::Argument>(varVal)) {
                    if (arg->getType()->isPointerTy()) {
                        _builder.CreateStore(val, arg);
                        return nullptr;
                    }
                    llvm::AllocaInst *alloca = _builder.CreateAlloca(arg->getType(), nullptr, arg->getName());
                    _builder.CreateStore(val, alloca);
                    return nullptr;
                }

                llvm::Value *targetAddr = varVal;
                ASTType currentASTType = type;
                if (vas->GetDerefDepth() > 0) {
                    if (llvm::isa<llvm::AllocaInst>(targetAddr) || llvm::isa<llvm::GlobalVariable>(targetAddr)) {
                        targetAddr = _builder.CreateLoad(llvmType, targetAddr, varName + ".addr");
                    }
                }
                for (unsigned char dd = vas->GetDerefDepth(); dd > 1; --dd) {
                    createCheckForNil(targetAddr, vas->GetStartLoc());
                    targetAddr = _builder.CreateLoad(_builder.getPtrTy(), targetAddr, varName + ".deref");
                    currentASTType = currentASTType.Deref();
                }
                if (vas->GetDerefDepth() > 0) {
                    createCheckForNil(targetAddr, vas->GetStartLoc());
                }
                for (unsigned char dd = vas->GetDerefDepth(); dd > 0; --dd, type.Deref());

                if (type.GetTypeKind() == ASTTypeKind::Trait) {
                    std::string concreteStructName = resolveStructName(vas->GetExpr());
                    llvm::Type *traitLLVMTy = llvmType;
                    val = castToTrait(val, traitLLVMTy, concreteStructName);
                }
                
                ASTType finalType = type;
                val = implicitlyCast(val, typeToLLVM(finalType));
                _builder.CreateStore(val, targetAddr);
                return nullptr;
            }
            varsCopy.pop();
        }
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitFunDeclStmt(FunDeclStmt *fds) {
        llvm::Function *fun = _functions.at(fds->GetName() == "main" ? "main" : getMangledName(fds->GetName()));
        llvm::BasicBlock *entry = llvm::BasicBlock::Create(_context, "entry", fun);
        _builder.SetInsertPoint(entry);
        _vars.push({});
        _funRetsTypes.push(fun->getReturnType());
        int index = 0;
        for (auto &arg : fun->args()) {
            arg.setName(fds->GetArgs()[index].GetName());
            _vars.top().emplace(arg.getName(), std::make_tuple(&arg, arg.getType(), fds->GetArgs()[index].GetType()));
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
        _vars.push({});
        if (fls->GetIndexator()) {
            Visit(fls->GetIndexator());
        }

        _builder.CreateBr(condBB);
        _builder.SetInsertPoint(condBB);
        llvm::Value *cond = Visit(fls->GetCondition());

        _builder.CreateCondBr(cond, bodyBB, exitBB);
        _builder.SetInsertPoint(bodyBB);
        _loopDeth.push({ exitBB, iterationBB });
        for (auto &stmt : fls->GetBody()) {
            Visit(stmt);
        }
        _loopDeth.pop();

        _builder.CreateBr(iterationBB);
        _builder.SetInsertPoint(iterationBB);
        if (fls->GetIteration()) {
            Visit(fls->GetIteration());
        }
        _vars.pop();

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
        bool oldLoad = createLoad;
        createLoad = fas->GetObjType().IsPointer();
        llvm::Value *obj = Visit(fas->GetObject());
        createLoad = oldLoad;

        ASTType objType = fas->GetObjType();
        if (!obj) {
            if (objType.GetTypeKind() == ASTTypeKind::Mod) {
                Module *mod = objType.GetModule();
                if (mod->Variables.count(fas->GetName())) {
                    Module *oldMod = _curMod;
                    _curMod = mod;
                    VarAsgnStmt *vas = new VarAsgnStmt(fas->GetName(), fas->GetExpr(), fas->GetAccess(), fas->GetStartLoc(), fas->GetEndLoc());
                    VisitVarAsgnStmt(vas);
                    delete vas;
                    _curMod = oldMod;
                    return nullptr;
                }
            }
            return nullptr;
        }
        if (objType.IsPointer()) {
            for (int i = 0; i < objType.GetPointerDepth() - 1; ++i) {
                createCheckForNil(obj, fas->GetStartLoc());
                obj = _builder.CreateLoad(_builder.getPtrTy(), obj, "ptr.deref");
            }
            createCheckForNil(obj, fas->GetStartLoc());
        }
        else if (objType.GetTypeKind() == ASTTypeKind::Struct) {
            if (!obj->getType()->isPointerTy()) {
                llvm::AllocaInst *tmp = _builder.CreateAlloca(obj->getType());
                _builder.CreateStore(obj, tmp);
                obj = tmp;
            }
        }

        Struct s = _structs.at(getMangledName(fas->GetObjType()));
        Field field = s.Fields.at(fas->GetName());
        llvm::Value *gep = _builder.CreateStructGEP(s.Type, obj, field.Index);
        llvm::Value *val = Visit(fas->GetExpr());
        val = implicitlyCast(val, field.Type);
        _builder.CreateStore(val, gep);
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitImplStmt(ImplStmt *is) {
        Struct s = _structs.at(getMangledName(is->GetStructType()));
        for (auto &stmt : is->GetBody()) {
            FunDeclStmt *method = llvm::cast<FunDeclStmt>(stmt);
            llvm::Function *fun = _functions.at(s.MangledName + "." + method->GetName());
            llvm::BasicBlock *entry = llvm::BasicBlock::Create(_context, "entry", fun);
            _builder.SetInsertPoint(entry);
            _vars.push({});
            _funRetsTypes.push(fun->getReturnType());
            ASTType thisType = is->GetStructType();
            int index = 0;
            for (auto &arg : fun->args()) {
                arg.setName(index == 0 ? "this" : method->GetArgs()[index - 1].GetName());
                _vars.top().emplace(arg.getName(), std::make_tuple(&arg, arg.getType(), index > 0 ? method->GetArgs()[index - 1].GetType() : thisType));
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
        expr->SetObjType(mcs->GetObjType());
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
        llvm::Value *val = Visit(es->GetExpr());
        llvm::Type *type = val->getType();
        if (type->isIntegerTy()) {
            unsigned bitWidth = type->getIntegerBitWidth();

            switch (bitWidth) {
                case 1: {
                    llvm::Constant *trueStr = getOrCreateGlobalString("true", "str.true");
                    llvm::Constant *falseStr = getOrCreateGlobalString("false", "str.false");

                    llvm::Value *selectedStr = _builder.CreateSelect(val, trueStr, falseStr, "bool.str");

                    llvm::Constant *fmtStr = getOrCreateGlobalString("%s", "printf.format");

                    _builder.CreateCall(_module->getFunction("printf"), { fmtStr, selectedStr });
                    return nullptr;
                }
                case 8:
                    format = "%c";
                    break;
                case 32:
                    format = "%d";
                    break;
                case 64:
                    format = "%lld";
                    break;
                default:
                    val = implicitlyCast(val, _builder.getInt64Ty());
                    format = "%lld";
                    break;
            }
        }
        else if (type->isFloatingPointTy()) {
            format = "%g";
            if (type->isFloatTy()) {
                val = _builder.CreateFPExt(val, llvm::Type::getDoubleTy(_context));
            }
        }
        else if (type->isPointerTy()) {
            if (es->GetExprType().GetTypeKind() == ASTTypeKind::Char) {
                format = "%s";
            }
            else {
                format = "%p";
            }
        }
        _builder.CreateCall(_module->getFunction("printf"), { getOrCreateGlobalString(format, "printf.format"), val });
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitDelStmt(DelStmt *ds) {
        bool oldLoad = createLoad;
        createLoad = false;
        llvm::Value *ptr = Visit(ds->GetExpr());
        createLoad = oldLoad;
        _builder.CreateCall(_module->getFunction("free"), { _builder.CreateLoad(_builder.getPtrTy(), ptr) });
        _builder.CreateStore(llvm::ConstantPointerNull::get(llvm::PointerType::get(_context, 0)), ptr);
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitImportStmt(ImportStmt *is) {
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitModDeclStmt(ModDeclStmt *mds) {
        return nullptr;
    }

    llvm::Value *
    CodeGen::VisitBinaryExpr(BinaryExpr *be) {
        llvm::Value *lhs = Visit(be->GetLHS());
        llvm::Value *rhs = Visit(be->GetRHS());
        llvm::Type *lhsType = lhs->getType();
        llvm::Type *rhsType = rhs->getType();

        bool leftIsPtr = lhsType->isPointerTy();
        bool rightIsPtr = rhsType->isPointerTy();
        int numPointers = leftIsPtr + rightIsPtr;
        if (numPointers == 1 && (be->GetOp().GetKind() == TkPlus || be->GetOp().GetKind() == TkMinus)) {
            llvm::Value *ptrVal = leftIsPtr ? lhs : rhs;
            llvm::Value *intVal = leftIsPtr ? rhs : lhs;
            intVal = implicitlyCast(intVal, _builder.getInt64Ty());
            bool subtract = be->GetOp().GetKind() == TkMinus;
            if (subtract) {
                intVal = _builder.CreateNeg(intVal, "neg.offset");
            }
            ASTType resultASTType = ASTType::GetCommon(be->GetLHSType(), be->GetRHSType()).Deref();
            llvm::Type *pointeeLLVMTy = typeToLLVM(resultASTType);
            llvm::Value *gep = _builder.CreateInBoundsGEP(pointeeLLVMTy, ptrVal, { intVal }, "ptr.arith");
            return gep;
        }
        else if (numPointers == 2 && (be->GetOp().GetKind() == TkPlus || be->GetOp().GetKind() == TkMinus)) {
            ASTType leftASTType = be->GetLHSType();
            llvm::Type *pointeeLLVMTy = typeToLLVM(leftASTType.Deref());
            llvm::Value *leftInt = _builder.CreatePtrToInt(lhs, _builder.getInt64Ty());
            llvm::Value *rightInt = _builder.CreatePtrToInt(rhs, _builder.getInt64Ty());
            llvm::Value *diff = _builder.CreateSub(leftInt, rightInt, "ptr.diff.bytes");
            const llvm::DataLayout &dl = _module->getDataLayout();
            uint64_t elemSize = dl.getTypeAllocSize(pointeeLLVMTy);
            if (elemSize > 1) {
                llvm::Value *sizeVal = _builder.getInt64(elemSize);
                diff = _builder.CreateExactSDiv(diff, sizeVal, "ptr.diff.elements");
            }
            ASTType resultASTType = ASTType::GetCommon(be->GetLHSType(), be->GetRHSType()).Deref();
            return implicitlyCast(diff, typeToLLVM(resultASTType));
        }

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
        std::string varName = _vars.size() == 1 ? getMangledName(ve->GetName()) : ve->GetName();
        while (!varsCopy.empty()) {
            if (auto var = varsCopy.top().find(varName); var != varsCopy.top().end()) {
                auto &[varVal, llvmType, type] = var->second;
                if (createLoad) {
                    if (auto glob = llvm::dyn_cast<llvm::GlobalVariable>(varVal)) {
                        if (_vars.size() == 1) {
                            return glob->getInitializer();
                        }
                        return _builder.CreateLoad(glob->getValueType(), glob, var->first + ".load");
                    }
                    if (auto local = llvm::dyn_cast<llvm::AllocaInst>(varVal)) {
                        return _builder.CreateLoad(local->getAllocatedType(), local, var->first + ".load");
                    }
                    if (auto arg = llvm::dyn_cast<llvm::Argument>(varVal)) {
                        if (arg->getName() == "this") {
                            std::string structName = resolveStructName(ve);
                            return _builder.CreateLoad(_structs.at(structName).Type, arg, var->first + ".load");
                        }
                    }
                }
                return varVal;
            }
            varsCopy.pop();
        }
        return nullptr;
    }
    
    llvm::Value *
    CodeGen::VisitLiteralExpr(LiteralExpr *le) {
        switch (le->GetVal().GetType().GetTypeKind()) {
            #define CONST_INT(fun, field) llvm::ConstantInt::get(llvm::Type::fun(_context), le->GetVal().GetData().field)
            #define CONST_FP(fun, field) llvm::ConstantFP::get(llvm::Type::fun(_context), le->GetVal().GetData().field)
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
        llvm::Function *fun = _functions.at(fce->GetName() == "main" ? "main" : getMangledName(fce->GetName()));
        std::vector<llvm::Value *> args(fce->GetArgs().size());
        for (int i = 0; i < fce->GetArgs().size(); ++i) {
            bool oldLoad = createLoad;
            createLoad = true;
            llvm::Value *val = Visit(fce->GetArgs()[i]);
            createLoad = oldLoad;
            llvm::Type *expectedType = fun->getFunctionType()->getParamType(i);

            if (expectedType->isStructTy() && expectedType->getStructNumElements() == 2 && 
                !val->getType()->isPointerTy()) {
                std::string structName = resolveStructName(fce->GetArgs()[i]);
                std::string traitName = getMangledName(_funArgsTypes.at(fce->GetName() == "main" ? "main" : getMangledName(fce->GetName()))[i]);

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
        if (fun->getReturnType() == _builder.getVoidTy()) {
            return _builder.CreateCall(fun, args);
        }
        return _builder.CreateCall(fun, args, fun->getName() + ".call");
    }

    llvm::Value *
    CodeGen::VisitStructExpr(StructExpr *se) {
        Struct s = _structs.at(getMangledName(se->GetType()));
        if (_vars.size() != 1) {
            llvm::AllocaInst *alloca = _builder.CreateAlloca(s.Type, nullptr, s.MangledName + ".alloca");
            for (int i = 0; i < se->GetInitializer().size(); ++i) {
                std::string name = se->GetInitializer()[i].first;
                llvm::Value *fieldPtr = _builder.CreateStructGEP(s.Type, alloca, s.Fields.at(name).Index, name + ".gep");
                llvm::Value *val = Visit(se->GetInitializer()[i].second);
                val = implicitlyCast(val, s.Fields.at(se->GetInitializer()[i].first).Type);
                _builder.CreateStore(val, fieldPtr);
                s.Fields.at(name).ManualInitialized = true;
            }
            for (auto &field : s.Fields) {
                if (!field.second.ManualInitialized) {
                    llvm::Value *fieldPtr = _builder.CreateStructGEP(s.Type, alloca, field.second.Index, field.second.Name + ".gep");
                    llvm::Value *val;
                    if (field.second.ASTType.GetTypeKind() == ASTTypeKind::Struct) {
                        if (field.second.ASTType.IsPointer()) {
                            val = llvm::ConstantPointerNull::get(_builder.getPtrTy());
                        }
                        else {
                            val = defaultStructConst(field.second.ASTType);
                        }
                    }
                    else {
                        val = llvm::Constant::getNullValue(field.second.Type);
                    }
                    _builder.CreateStore(val, fieldPtr);
                }
            }
            return _builder.CreateLoad(s.Type, alloca, s.MangledName + ".alloca.load");
        }
        else {
            std::vector<llvm::Constant *> fieldValues(s.Type->getNumElements(), nullptr);
            for (const auto &initPair : se->GetInitializer()) {
                const std::string &name = initPair.first;
                auto &field = s.Fields.at(name);
                llvm::Value *val = Visit(initPair.second);
                fieldValues[field.Index] = llvm::cast<llvm::Constant>(val);
                field.ManualInitialized = true;
            }
            for (auto &[name, field] : s.Fields) {
                if (fieldValues[field.Index] == nullptr) {
                    // TODO: add support of default values in structure: llvm::dyn_cast<llvm::Constant>(field.DefaultExpr ? Visit(field.DefaultExpr) : nullptr);

                    if (!fieldValues[field.Index]) {
                        if (field.ASTType.GetTypeKind() == ASTTypeKind::Struct) {
                            fieldValues[field.Index] = field.ASTType.IsPointer() ? llvm::ConstantPointerNull::get(_builder.getPtrTy())
                                                                                 : llvm::dyn_cast<llvm::Constant>(defaultStructConst(field.ASTType));
                        }
                        else {
                            fieldValues[field.Index] = llvm::Constant::getNullValue(field.Type);
                        }
                    }
                }
            }
            return llvm::ConstantStruct::get(s.Type, fieldValues);
        }
    }

    llvm::Value *
    CodeGen::VisitFieldAccessExpr(FieldAccessExpr *fae) {
        bool oldLoad = createLoad;
        createLoad = fae->GetObjType().IsPointer();
        llvm::Value *obj = Visit(fae->GetObject());
        createLoad = oldLoad;
        
        ASTType objASTType = fae->GetObjType();
        if (!obj) { // maybe it is module
            if (objASTType.GetTypeKind() == ASTTypeKind::Mod) {
                Module *mod = objASTType.GetModule();
                if (mod->Variables.count(fae->GetName())) {
                    Module *oldMod = _curMod;
                    _curMod = mod;
                    VarExpr *ve = new VarExpr(getMangledName(fae->GetName()), fae->GetStartLoc(), fae->GetEndLoc());
                    llvm::Value *val = VisitVarExpr(ve);
                    delete ve;
                    _curMod = oldMod;
                    return val;
                }
                return nullptr;
            }
        }
        if (objASTType.IsPointer()) {
            for (int i = 0; i < objASTType.GetPointerDepth() - 1; ++i) {
                createCheckForNil(obj, fae->GetStartLoc());
                obj = _builder.CreateLoad(_builder.getPtrTy(), obj, "ptr.deref");
            }
            createCheckForNil(obj, fae->GetStartLoc());
        }
        else if (objASTType.GetTypeKind() == ASTTypeKind::Struct) {
            if (!obj->getType()->isPointerTy()) {
                llvm::AllocaInst *tempAlloca = _builder.CreateAlloca(obj->getType(), nullptr, "struct.tmp");
                _builder.CreateStore(obj, tempAlloca);
                obj = tempAlloca;
            }
        }

        Struct s = _structs.at(getMangledName(fae->GetObjType()));
        Field field = s.Fields.at(fae->GetName());
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
        if (createLoad) {
            return _builder.CreateLoad(s.Type->getTypeAtIndex(field.Index), gep, s.MangledName + "." + fae->GetName() + ".load");
        }
        return gep;
    }

    llvm::Value *
    CodeGen::VisitMethodCallExpr(MethodCallExpr *mce) {
        bool oldLoad = createLoad;
        createLoad = mce->GetObjType().IsPointer();
        llvm::Value *obj = Visit(mce->GetObject());
        createLoad = oldLoad;

        ASTType objType = mce->GetObjType();
        if (!obj) { // maybe it is module
            if (objType.GetTypeKind() == ASTTypeKind::Mod) {
                Module *oldMod = _curMod;
                _curMod = objType.GetModule();
                FunCallExpr *expr = new FunCallExpr(mce->GetName(), mce->GetArgs(), mce->GetStartLoc(), mce->GetEndLoc());
                llvm::Value *val = VisitFunCallExpr(expr);
                delete expr;
                _curMod = oldMod;
                return val;
            }
        }
        if (objType.IsPointer()) {
            for (int i = 0; i < objType.GetPointerDepth() - 1; ++i) {
                createCheckForNil(obj, mce->GetStartLoc());
                obj = _builder.CreateLoad(_builder.getPtrTy(), obj, "ptr.deref");
            }
            createCheckForNil(obj, mce->GetStartLoc());
        }
        else if (objType.GetTypeKind() == ASTTypeKind::Struct) {
            if (!obj->getType()->isPointerTy()) {
                llvm::AllocaInst *tmp = _builder.CreateAlloca(obj->getType());
                _builder.CreateStore(obj, tmp);
                obj = tmp;
            }
        }
        std::string typeName = getMangledName(objType);
        if (objType.GetTypeKind() == ASTTypeKind::Trait) {
            llvm::Value *fatPtr = obj;
            if (obj->getType()->isPointerTy() && !llvm::isa<llvm::Argument>(obj)) {
                fatPtr = _builder.CreateLoad(typeToLLVM(objType), obj, "trait.ptr");
            }

            llvm::Value *thisPtr = _builder.CreateExtractValue(fatPtr, 0, "trait.this");
            llvm::Value *vtablePtr = _builder.CreateExtractValue(fatPtr, 1, "trait.vtable");

            Trait &t = _traits.at(typeName);
            int methodIdx = -1;
            for (int i = 0; i < t.Methods.size(); ++i) {
                if (t.Methods[i].first == mce->GetName()) {
                    methodIdx = i;
                    break;
                }
            }

            llvm::Type *ptrTy = _builder.getPtrTy();
            llvm::Value *gep = _builder.CreateConstGEP1_32(ptrTy, vtablePtr, methodIdx);
            llvm::Value *funPtr = _builder.CreateLoad(ptrTy, gep, "vfun.addr");

            std::vector<llvm::Value *> args = { thisPtr };
            for (auto *arg : mce->GetArgs()) {
                args.push_back(Visit(arg));
            }

            auto &m = t.Methods[methodIdx].second;
            llvm::FunctionType *FTy = llvm::FunctionType::get(m.RetType, m.Args, false);
            return _builder.CreateCall(FTy, funPtr, args);
        }
        
        std::string structName = getMangledName(objType);
        if (structName.empty() && obj->getType()->isStructTy()) {
            structName = obj->getType()->getStructName().str();
            /* TODO: is it necessary???
            int dotPos = structName.find_last_of('.');
            if (dotPos != std::string::npos) {
                structName = structName.substr(0, dotPos);
            }
            */
        }

        if (_traits.count(structName)) {
            llvm::Value *thisPtr = _builder.CreateExtractValue(obj, 0, "trait.this");
            llvm::Value *vtablePtr = _builder.CreateExtractValue(obj, 1, "trait.vtable");

            Trait &t = _traits.at(structName);
            int methodIndex = -1;
            llvm::Type* retType = nullptr;

            for (int i = 0; i < t.Methods.size(); ++i) {
                if (t.Methods[i].first == mce->GetName()) {
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
            llvm::Value *funRaw = _builder.CreateLoad(voidPtrTy, gep, "vfun.ptr");

            std::vector<llvm::Value *> args(mce->GetArgs().size() + 1);
            args[0] = thisPtr;
            oldLoad = createLoad;
            createLoad = true;
            for (int i = 0; i < mce->GetArgs().size(); ++i) {
                args[i + 1] = implicitlyCast(Visit(mce->GetArgs()[i]), t.Methods[methodIndex].second.Args[i]);
            }
            createLoad = oldLoad;
            llvm::FunctionType *method = llvm::FunctionType::get(retType, t.Methods[methodIndex].second.Args, false);

            return _builder.CreateCall(method, funRaw, args);
        }

        std::string fullName = structName + "." + mce->GetName();
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

        const std::vector<ASTType> &paramASTTypes = _funArgsTypes.at(fullName);
        for (int i = 0; i < mce->GetArgs().size(); ++i) {
            oldLoad = createLoad;
            createLoad = true;
            llvm::Value *val = Visit(mce->GetArgs()[i]);
            createLoad = oldLoad;
            llvm::Type *expectedType = fun->getFunctionType()->getParamType(i + 1);

            if (paramASTTypes[i + 1].GetTypeKind() == ASTTypeKind::Trait) {
                std::string structName = resolveStructName(mce->GetArgs()[i]);
                std::string traitName = getMangledName(_funArgsTypes.at(fullName)[i + 1]);

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

    llvm::Value *
    CodeGen::VisitNilExpr(NilExpr *ne) {
        return llvm::ConstantPointerNull::get(llvm::PointerType::get(_context, 0));
    }

    llvm::Value *
    CodeGen::VisitDerefExpr(DerefExpr *de) {
        bool oldLoad = createLoad;
        createLoad = true;
        llvm::Value *ptrVal = Visit(de->GetExpr());
        createLoad = oldLoad;
        if (!ptrVal->getType()->isPointerTy()) {
            return ptrVal;
        }
        if (_vars.size() != 1) {
            createCheckForNil(ptrVal, de->GetStartLoc());
        }
        if (createLoad) {
            ptrVal = _builder.CreateLoad(typeToLLVM(de->GetExprType().Deref()), ptrVal, "deref.load");
        }
        return ptrVal;
    }

    llvm::Value *
    CodeGen::VisitRefExpr(RefExpr *re) {
        bool oldLoad = createLoad;
        createLoad = false;
        llvm::Value *ptr = Visit(re->GetExpr());
        createLoad = oldLoad;
        return ptr;
    }

    llvm::Value *
    CodeGen::VisitNewExpr(NewExpr *ne) {
        llvm::Value *size = _builder.getInt64(_module->getDataLayout().getTypeAllocSize(typeToLLVM(ne->GetType())));
        llvm::Value *ptr = _builder.CreateCall(_module->getFunction("malloc"), { size });
        if (ne->GetStructExpr()) {
            llvm::Value *se = VisitStructExpr(ne->GetStructExpr());
            _builder.CreateStore(se, ptr);
        }
        return ptr;
    }

    void
    CodeGen::declareRuntimeFunctions() {
#define DECL(n, t, hasVariadic, ...)                                                                        \
        llvm::Function::Create(llvm::FunctionType::get(_builder.t, { __VA_ARGS__ }, hasVariadic),           \
                llvm::GlobalValue::ExternalLinkage, n, *_module);
#define DECL2(n, t, hasVariadic)                                                                            \
        llvm::Function::Create(llvm::FunctionType::get(_builder.t, { }, hasVariadic),                       \
                llvm::GlobalValue::ExternalLinkage, n, *_module);

        DECL("printf",  getInt32Ty(),   true,   _builder.getPtrTy()     );
        DECL2("abort",  getVoidTy(),    false                           );
        DECL("malloc",  getPtrTy(),     false,  _builder.getInt64Ty()   );
        DECL("free",    getVoidTy(),    false,  _builder.getPtrTy()     );

#undef DECL2
#undef DECL
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
            /* TODO: is it necessary???
            int dotPos = traitName.find_last_of('.');
            if (dotPos != std::string::npos) {
                traitName = traitName.substr(0, dotPos);
            }
            */

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
        llvm::Type *base;
        switch (type.GetTypeKind()) {
            #define TYPE(fun) llvm::Type::fun(_context);
            case ASTTypeKind::Bool:
                base = TYPE(getInt1Ty);
                break;
            case ASTTypeKind::Char:
                base = TYPE(getInt8Ty);
                break;
            case ASTTypeKind::I16:
                base = TYPE(getInt16Ty);
                break;
            case ASTTypeKind::I32:
                base = TYPE(getInt32Ty);
                break;
            case ASTTypeKind::I64:
                base = TYPE(getInt64Ty);
                break;
            case ASTTypeKind::F32:
                base = TYPE(getFloatTy);
                break;
            case ASTTypeKind::F64:
                base = TYPE(getDoubleTy);
                break;
            case ASTTypeKind::Struct:
                base = _structs.at(getMangledName(type)).Type;
                break;
            case ASTTypeKind::Trait: {
                llvm::StructType *existingType = llvm::StructType::getTypeByName(_context, getMangledName(type));
                if (existingType) {
                    base = existingType;
                    break;
                }
                llvm::Type *voidPtrTy = llvm::PointerType::get(_context, 0);
                llvm::Type *vtablePtrTy = llvm::PointerType::get(voidPtrTy, 0);
                base = llvm::StructType::create(_context, { voidPtrTy, vtablePtrTy }, getMangledName(type));
                break;
            }
            case ASTTypeKind::Noth:
                base = TYPE(getVoidTy);
                break;
            #undef TYPE
        }
        for (unsigned char pd = type.GetPointerDepth(); pd > 0; --pd, base = llvm::PointerType::get(base, 0));
        return base;
    }

    llvm::Value *
    CodeGen::defaultStructConst(ASTType type) {
        Struct s = _structs.at(getMangledName(type));
        std::vector<llvm::Constant *> fieldValues(s.Type->getNumElements(), nullptr);

        for (auto &[name, field] : s.Fields) {
            llvm::Constant *val;
            /* TODO: return support of default values in structure
            if (field.DefaultExpr) {
                val = llvm::cast<llvm::Constant>(Visit(field.DefaultExpr));
            }
            else 
            */
            if (field.ASTType.GetTypeKind() == ASTTypeKind::Struct) {
                val = field.ASTType.IsPointer() ? llvm::ConstantPointerNull::get(_builder.getPtrTy()) : llvm::dyn_cast<llvm::Constant>(defaultStructConst(field.ASTType));
            }
            else {
                val = llvm::Constant::getNullValue(field.Type);
            }
            fieldValues[field.Index] = val;
        }
        return llvm::ConstantStruct::get(s.Type, fieldValues);
    }

    std::string
    CodeGen::resolveStructName(Expr *expr) {
        switch (expr->GetKind()) {
            case NkVarExpr: {
                std::string name = _vars.size() == 0 ? getMangledName(llvm::dyn_cast<VarExpr>(expr)->GetName()) : llvm::dyn_cast<VarExpr>(expr)->GetName();
                auto varsCopy = _vars;
                while (!varsCopy.empty()) {
                    for (auto var : varsCopy.top()) {
                        if (var.first == name) {
                            auto &[varVal, llvmType, type] = var.second;
                            if (auto *glob = llvm::dyn_cast<llvm::GlobalVariable>(varVal)) {
                                if (auto *metadata = glob->getMetadata("struct_name")) {
                                    if (auto *mdStr = llvm::dyn_cast<llvm::MDString>(metadata->getOperand(0))) {
                                        return mdStr->getString().str();
                                    }
                                }
                            } 
                            else if (auto *local = llvm::dyn_cast<llvm::AllocaInst>(varVal)) {
                                if (auto *metadata = local->getMetadata("struct_name")) {
                                    if (auto *mdStr = llvm::dyn_cast<llvm::MDString>(metadata->getOperand(0))) {
                                        return mdStr->getString().str();
                                    }
                                }
                            }
                            else if (auto *arg = llvm::dyn_cast<llvm::Argument>(varVal)) {
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
                FunCallExpr *fce = llvm::dyn_cast<FunCallExpr>(expr);
                std::string name = fce->GetName() == "main" ? "main" : getMangledName(fce->GetName());
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
                std::string parentStructName = getMangledName(fae->GetObjType());
                if (parentStructName.empty()) {
                    return "";
                }

                if (_structs.count(parentStructName)) {
                    Struct &s = _structs.at(parentStructName);
                    std::string fieldName = fae->GetName();
                    
                    if (s.Fields.count(fieldName)) {
                        Field &f = s.Fields.at(fieldName);
                        if (f.ASTType.GetTypeKind() == ASTTypeKind::Struct) {
                            return getMangledName(f.ASTType);
                        }
                    }
                }
                return "";
            }
            case NkMethodCallExpr: {
                MethodCallExpr *mce = llvm::dyn_cast<MethodCallExpr>(expr);
                std::string parentStructName = getMangledName(mce->GetObjType());
                if (parentStructName.empty()) {
                    return "";
                }

                llvm::Function *fun = _functions.at(parentStructName + "." + mce->GetName());
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

    void
    CodeGen::createCheckForNil(llvm::Value *ptr, llvm::SMLoc loc) {
        auto [line, col] = _srcMgr.getLineAndColumn(loc);

        std::string msgStr = "Error: Null pointer dereference at " + _module->getSourceFileName() + ":" +
                             std::to_string(line) + ":" + std::to_string(col) + "!\n";

        llvm::BasicBlock *currentBB = _builder.GetInsertBlock();
        llvm::Function *parentFun = currentBB->getParent();

        llvm::BasicBlock *notNullBB = llvm::BasicBlock::Create(_context, "not.null", parentFun);
        llvm::BasicBlock *nullBB = llvm::BasicBlock::Create(_context, "null.error", parentFun);

        llvm::Value *isNull = _builder.CreateIsNull(ptr, "is.null");

        _builder.CreateCondBr(isNull, nullBB, notNullBB);
        _builder.SetInsertPoint(nullBB);

        _builder.CreateCall(_module->getFunction("printf"), { getOrCreateGlobalString(msgStr, "null.err.msg") }, "printf_call");

        _builder.CreateCall(_module->getFunction("abort"));
        _builder.CreateUnreachable();

        _builder.SetInsertPoint(notNullBB);
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

    llvm::Value *
    CodeGen::castToTrait(llvm::Value *src, llvm::Type *traitType, const std::string &structName) {
        llvm::Value *fatPtr = llvm::UndefValue::get(traitType);
        
        llvm::Value *dataPtr;
        if (src->getType()->isPointerTy()) {
            dataPtr = _builder.CreateBitCast(src, _builder.getPtrTy());
        }
        else {
            llvm::AllocaInst *tmp = _builder.CreateAlloca(src->getType());
            _builder.CreateStore(src, tmp);
            dataPtr = tmp;
        }
        fatPtr = _builder.CreateInsertValue(fatPtr, dataPtr, 0);

        std::string traitName = traitType->getStructName().str();
        llvm::Value *vtable = getOrCreateVTable(structName, traitName);
        llvm::Value *vtablePtr = _builder.CreateBitCast(vtable, _builder.getPtrTy());
        fatPtr = _builder.CreateInsertValue(fatPtr, vtablePtr, 1);
        return fatPtr;
    }

    llvm::Constant *
    CodeGen::getOrCreateGlobalString(std::string val, std::string name) {
        if (auto it = _strPool.find(val); it != _strPool.end()) {
            return it->second;
        }
        _strPool[val] = _builder.CreateGlobalString(val, name);
        return _strPool[val];
    }

    std::string
    CodeGen::getMangledName(std::string name, char sep, Module *mod) {
        if (!mod) {
            mod = _curMod;
        }
        return mod->ToString(sep) + sep + name;
    }

    std::string
    CodeGen::getMangledName(ASTType type, char sep) {
        if (type.GetTypeKind() != ASTTypeKind::Mod && type.GetTypeKind() != ASTTypeKind::Struct && type.GetTypeKind() != ASTTypeKind::Trait) {
            return type.GetVal();
        }
        return getMangledName(type.GetVal(), sep, type.GetModule());
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
