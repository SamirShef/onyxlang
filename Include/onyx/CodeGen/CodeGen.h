#pragma once
#include <llvm/IR/Module.h>
#include <onyx/AST/Visitor.h>
#include <onyx/Basic/DiagnosticEngine.h>
#include <llvm/IR/IRBuilder.h>
#include <stack>

namespace onyx {
    class CodeGen : public ASTVisitor<CodeGen, llvm::Value *> {
        llvm::LLVMContext _context;
        std::unique_ptr<llvm::Module> _module;
        llvm::IRBuilder<> _builder;

        std::stack<std::unordered_map<std::string, llvm::Value *>> _vars;

        std::unordered_map<std::string, llvm::Function *> functions;
        std::stack<llvm::Type *> funRetsTypes;

    public:
        explicit CodeGen(std::string fileName) : _context(), _builder(_context),
                                                 _module(std::make_unique<llvm::Module>(fileName, _context)) {
            _vars.push({});
        }

        std::unique_ptr<llvm::Module>
        GetModule() {
            return std::move(_module);
        }

        llvm::Value *
        visitVarDeclStmt(VarDeclStmt *vds);

        llvm::Value *
        visitVarAsgnStmt(VarAsgnStmt *vas);

        llvm::Value *
        visitFunDeclStmt(FunDeclStmt *fds);

        llvm::Value *
        visitFunCallStmt(FunCallStmt *fcs);

        llvm::Value *
        visitRetStmt(RetStmt *rs);
                
        llvm::Value *
        visitBinaryExpr(BinaryExpr *be);
                
        llvm::Value *
        visitUnaryExpr(UnaryExpr *ue);
                
        llvm::Value *
        visitVarExpr(VarExpr *ve);
                
        llvm::Value *
        visitLiteralExpr(LiteralExpr *le);

        llvm::Value *
        visitFunCallExpr(FunCallExpr *fce);

        llvm::Type *
        getCommonType(llvm::Type *left, llvm::Type *right);
        
        llvm::Value *
        implicitlyCast(llvm::Value *src, llvm::Type *expectType);

        llvm::SMRange
        getRange(llvm::SMLoc start, int len) const;

        llvm::Type *
        typeKindToLLVM(ASTTypeKind kind);
    };
}