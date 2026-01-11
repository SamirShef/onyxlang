#include <onyx/CodeGen/CodeGen.h>

namespace onyx {
    llvm::Value *
    CodeGen::visitVarDeclStmt(VarDeclStmt *vds) {
        llvm::Value *initializer = nullptr;
        if (vds->GetExpr()) {
            initializer = visit(vds->GetExpr());
            initializer = implicitlyCast(initializer, typeKindToLLVM(vds->GetType().GetTypeKind()));
        }
        else {
            initializer = llvm::Constant::getNullValue(typeKindToLLVM(vds->GetType().GetTypeKind()));
        }
        llvm::Value *var;
        if (_vars.size() == 1) {
            var = new llvm::GlobalVariable(*_module, typeKindToLLVM(vds->GetType().GetTypeKind()), vds->IsConst(), llvm::GlobalValue::ExternalLinkage, llvm::cast<llvm::Constant>(initializer), vds->GetName());
        }
        else {
            var = _builder.CreateAlloca(typeKindToLLVM(vds->GetType().GetTypeKind()), nullptr, vds->GetName());
            _builder.CreateStore(initializer, var);
        }
        _vars.top().emplace(vds->GetName().str(), var);
        return nullptr;
    }

    llvm::Value *
    CodeGen::visitFunDeclStmt(FunDeclStmt *fds) {
        std::vector<llvm::Type *> args(fds->GetArgs().size());
        for (int i = 0; i < fds->GetArgs().size(); ++i) {
            args[i] = typeKindToLLVM(fds->GetArgs()[i].GetType().GetTypeKind());
        }
        llvm::FunctionType *retType = llvm::FunctionType::get(typeKindToLLVM(fds->GetRetType().GetTypeKind()), args, false);
        llvm::Function *fun = llvm::Function::Create(retType, llvm::GlobalValue::ExternalLinkage, fds->GetName(), *_module);
        llvm::BasicBlock *entry = llvm::BasicBlock::Create(_context, "entry", fun);
        _builder.SetInsertPoint(entry);
        _vars.push({});
        funRetsTypes.push(fun->getReturnType());
        functions.emplace(fun->getName(), fun);
        int index = 0;
        for (auto &arg : fun->args()) {
            arg.setName(fds->GetArgs()[index].GetName());
            llvm::AllocaInst *alloca = _builder.CreateAlloca(arg.getType(), nullptr, arg.getName());
            _builder.CreateStore(&arg, alloca);
            _vars.top().emplace(arg.getName().str(), alloca);
            ++index;
        }
        for (auto &stmt : fds->GetBody()) {
            visit(stmt);
        }
        if (fds->GetRetType().GetTypeKind() == ASTTypeKind::Noth) {
            _builder.CreateRetVoid();
        }
        funRetsTypes.pop();
        _vars.pop();
        return nullptr;
    }

    llvm::Value *
    CodeGen::visitFunCallStmt(FunCallStmt *fcs) {
        visit(new FunCallExpr(fcs->GetName(), fcs->GetArgs(), fcs->GetStartLoc(), fcs->GetEndLoc()));
        return nullptr;
    }

    llvm::Value *
    CodeGen::visitRetStmt(RetStmt *rs) {
        if (rs->GetExpr()) {
            llvm::Value *val = visit(rs->GetExpr());
            return _builder.CreateRet(implicitlyCast(val, funRetsTypes.top()));
        }
        return _builder.CreateRetVoid();
    }
    
    llvm::Value *
    CodeGen::visitBinaryExpr(BinaryExpr *be) {
        llvm::Value *lhs = visit(be->GetLHS());
        llvm::Value *rhs = visit(be->GetRHS());
        llvm::Type *lhsType = lhs->getType();
        llvm::Type *rhsType = rhs->getType();
        llvm::Type *commonType = getCommonType(lhsType, rhsType);
        if (lhsType != commonType) {
            lhs = implicitlyCast(lhs, commonType);
            lhsType = lhs->getType();
        }
        else if (rhsType != commonType) {
            rhs = implicitlyCast(rhs, commonType);
            rhsType = rhs->getType();
        }
        switch (be->GetOp().GetKind()) {
            case TkPlus:
                if (commonType->isFloatingPointTy()) {
                    _builder.CreateFAdd(lhs, rhs);
                }
                return _builder.CreateAdd(lhs, rhs);
            case TkMinus:
                if (commonType->isFloatingPointTy()) {
                    _builder.CreateFSub(lhs, rhs);
                }
                return _builder.CreateSub(lhs, rhs);
            case TkStar:
                if (commonType->isFloatingPointTy()) {
                    _builder.CreateFMul(lhs, rhs);
                }
                return _builder.CreateMul(lhs, rhs);
            case TkSlash:
                if (commonType->isFloatingPointTy()) {
                    _builder.CreateFDiv(lhs, rhs);
                }
                return _builder.CreateSDiv(lhs, rhs);
            case TkPercent:
                if (commonType->isFloatingPointTy()) {
                    _builder.CreateFRem(lhs, rhs);
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
                    _builder.CreateFCmpOGT(lhs, rhs);
                }
                return _builder.CreateICmpSGT(lhs, rhs);
            case TkGtEq:
                if (commonType->isFloatingPointTy()) {
                    _builder.CreateFCmpOGE(lhs, rhs);
                }
                return _builder.CreateICmpSGE(lhs, rhs);
            case TkLt:
                if (commonType->isFloatingPointTy()) {
                    _builder.CreateFCmpOLT(lhs, rhs);
                }
                return _builder.CreateICmpSLT(lhs, rhs);
            case TkLtEq:
                if (commonType->isFloatingPointTy()) {
                    _builder.CreateFCmpOLE(lhs, rhs);
                }
                return _builder.CreateICmpSLE(lhs, rhs);
            case TkEqEq:
                if (commonType->isFloatingPointTy()) {
                    _builder.CreateFCmpOEQ(lhs, rhs);
                }
                return _builder.CreateICmpEQ(lhs, rhs);
            case TkNotEq:
                if (commonType->isFloatingPointTy()) {
                    _builder.CreateFCmpONE(lhs, rhs);
                }
                return _builder.CreateICmpNE(lhs, rhs);
            default: {}
        }
        return nullptr;
    }
    
    llvm::Value *
    CodeGen::visitUnaryExpr(UnaryExpr *ue) {
        llvm::Value *rhs = visit(ue->GetRHS());
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
    CodeGen::visitVarExpr(VarExpr *ve) {
        auto varsCopy = _vars;
        while (!varsCopy.empty()) {
            if (auto var = varsCopy.top().find(ve->GetName().str()); var != varsCopy.top().end()) {
                if (auto glob = llvm::dyn_cast<llvm::GlobalVariable>(var->second)) {
                    if (_vars.size() == 1) {
                        return glob->getInitializer();
                    }
                    return _builder.CreateLoad(glob->getValueType(), glob, var->first + ".load");
                }
                if (auto local = llvm::dyn_cast<llvm::AllocaInst>(var->second)) {
                    return _builder.CreateLoad(local->getAllocatedType(), local, var->first + ".load");
                }
                return var->second;
            }
            varsCopy.pop();
        }
        return nullptr;
    }
    
    llvm::Value *
    CodeGen::visitLiteralExpr(LiteralExpr *le) {
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
    CodeGen::visitFunCallExpr(FunCallExpr *fce) {
        std::vector<llvm::Value *> args(fce->GetArgs().size());
        for (int i = 0; i < fce->GetArgs().size(); ++i) {
            args[i] = visit(fce->GetArgs()[i]);
        }
        llvm::Function *fun = functions.at(fce->GetName().str());
        return _builder.CreateCall(fun, args, fce->GetName() + ".call");
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
        return nullptr;
    }

    llvm::SMRange
    CodeGen::getRange(llvm::SMLoc start, int len) const {
        return llvm::SMRange(start, llvm::SMLoc::getFromPointer(start.getPointer() + len));
    }

    llvm::Type *
    CodeGen::typeKindToLLVM(ASTTypeKind kind) {
        switch (kind) {
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
            case ASTTypeKind::Noth:
                return TYPE(getVoidTy);
            #undef TYPE
        }
    }
}