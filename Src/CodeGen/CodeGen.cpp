#include <onyx/CodeGen/CodeGen.h>

namespace onyx {
    llvm::Value *
    CodeGen::visitVarDeclStmt(VarDeclStmt *vds) {
        llvm::Value *initializer = nullptr;
        if (vds->GetExpr()) {
            initializer = visit(vds->GetExpr());
            // TODO: create implicitly cast initializer value to target type
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
        
    }

    llvm::Value *
    CodeGen::visitFunCallStmt(FunCallStmt *fcs) {
        
    }

    llvm::Value *
    CodeGen::visitRetStmt(RetStmt *rs) {
        
    }
    
    llvm::Value *
    CodeGen::visitBinaryExpr(BinaryExpr *be) {
        llvm::Value *lhs = visit(be->GetLHS());
        llvm::Value *rhs = visit(be->GetRHS());
        // TODO: create implicitly cast values to common type
        switch (be->GetOp().GetKind()) {
            // TODO: create logic for floating point numbers
            case TkPlus:
                return _builder.CreateAdd(lhs, rhs, "add.tmp");
            case TkMinus:
                return _builder.CreateSub(lhs, rhs, "sub.tmp");
            case TkStar:
                return _builder.CreateMul(lhs, rhs, "mul.tmp");
            case TkSlash:
                return _builder.CreateSDiv(lhs, rhs, "sdiv.tmp");
            case TkPercent:
                return _builder.CreateSRem(lhs, rhs, "srem.tmp");
            case TkLogAnd:
                return _builder.CreateLogicalAnd(lhs, rhs, "logical_and.tmp");
            case TkLogOr:
                return _builder.CreateLogicalOr(lhs, rhs, "logical_or.tmp");
            case TkAnd:
                return _builder.CreateAnd(lhs, rhs, "and.tmp");
            case TkOr:
                return _builder.CreateOr(lhs, rhs, "or.tmp");
            case TkGt:
                return _builder.CreateICmpSGT(lhs, rhs, "sgt.tmp");
            case TkGtEq:
                return _builder.CreateICmpSGE(lhs, rhs, "sge.tmp");
            case TkLt:
                return _builder.CreateICmpSLT(lhs, rhs, "sgt.tmp");
            case TkLtEq:
                return _builder.CreateICmpSLE(lhs, rhs, "sle.tmp");
            case TkEqEq:
                return _builder.CreateICmpEQ(lhs, rhs, "eq.tmp");
            case TkNotEq:
                return _builder.CreateICmpNE(lhs, rhs, "ne.tmp");
            default: {}
        }
        return nullptr;
    }
    
    llvm::Value *
    CodeGen::visitUnaryExpr(UnaryExpr *ue) {
        llvm::Value *rhs = visit(ue->GetRHS());
        switch (ue->GetOp().GetKind()) {
            case TkMinus:
                return _builder.CreateNeg(rhs, "neg.tmp");
            case TkBang:
                return _builder.CreateNot(rhs, "not.tmp");
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
                return CONST_INT(getInt8Ty, boolVal);
            case ASTTypeKind::I16:
                return CONST_INT(getInt16Ty, boolVal);
            case ASTTypeKind::I32:
                return CONST_INT(getInt32Ty, boolVal);
            case ASTTypeKind::I64:
                return CONST_INT(getInt64Ty, boolVal);
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