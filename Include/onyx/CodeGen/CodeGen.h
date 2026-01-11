#pragma once
#include <llvm/IR/Module.h>
#include <onyx/AST/Visitor.h>
#include <onyx/Basic/DiagnosticEngine.h>
#include <llvm/IR/IRBuilder.h>
#include <stack>

namespace onyx {
    class CodeGen : public ASTVisitor<CodeGen, llvm::Value *> {
        DiagnosticEngine &_diag;
        llvm::LLVMContext _context;
        std::unique_ptr<llvm::Module> _module;
        llvm::IRBuilder<> _builder;

        std::stack<std::unordered_map<std::string, llvm::Value *>> _vars;

    public:
        explicit CodeGen(DiagnosticEngine &diag, std::string fileName) : _diag(diag), _context(), _builder(_context),
                                                                         _module(std::make_unique<llvm::Module>(fileName, _context)) {
            _vars.push({});
            llvm::FunctionType *retType = llvm::FunctionType::get(typeKindToLLVM(ASTTypeKind::I32), false);
            llvm::Function *main = llvm::Function::Create(retType, llvm::GlobalValue::ExternalLinkage, "main", *_module);
            llvm::BasicBlock *entry = llvm::BasicBlock::Create(_context, "entry", main);
            _builder.SetInsertPoint(entry);
            _builder.CreateRet(llvm::Constant::getNullValue(typeKindToLLVM(ASTTypeKind::I32)));
        }

        std::unique_ptr<llvm::Module>
        GetModule() {
            return std::move(_module);
        }

        llvm::Value *
        visitVarDeclStmt(VarDeclStmt *vds);
                
        llvm::Value *
        visitBinaryExpr(BinaryExpr *be);
                
        llvm::Value *
        visitUnaryExpr(UnaryExpr *ue);
                
        llvm::Value *
        visitVarExpr(VarExpr *ve);
                
        llvm::Value *
        visitLiteralExpr(LiteralExpr *le);

        llvm::SMRange
        getRange(llvm::SMLoc start, int len) const;

        llvm::Type *
        typeKindToLLVM(ASTTypeKind kind);
    };
}