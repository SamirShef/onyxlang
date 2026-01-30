#pragma once
#include <onyx/AST/Visitor.h>
#include <onyx/Basic/DiagnosticEngine.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <stack>

namespace onyx {
    class CodeGen : public ASTVisitor<CodeGen, llvm::Value *> {
        llvm::LLVMContext _context;
        std::unique_ptr<llvm::Module> _module;
        llvm::IRBuilder<> _builder;

        std::stack<std::unordered_map<std::string, llvm::Value *>> _vars;

        std::unordered_map<std::string, llvm::Function *> _functions;
        std::stack<llvm::Type *> _funRetsTypes;

        std::stack<std::pair<llvm::BasicBlock *, llvm::BasicBlock *>> _loopDeth;    // first for break, second for continue

        struct Field {
            const llvm::StringRef Name;
            llvm::Type *Type;
            ASTType ASTType;
            llvm::Value *Val;
            bool ManualInitialized;
            long Index;
        };

        struct Struct {
            const llvm::StringRef Name;
            llvm::StructType *Type;
            std::unordered_map<std::string, Field> Fields;
        };
        std::unordered_map<std::string, Struct> _structs;

    public:
        explicit CodeGen(std::string fileName) : _context(), _builder(_context),
                                                 _module(std::make_unique<llvm::Module>(fileName, _context)) {
            _vars.push({});
        }

        std::unique_ptr<llvm::Module>
        GetModule() {
            return std::move(_module);
        }

        void
        DeclareFunctionsAndStructures(std::vector<Stmt *> &ast);

        llvm::Value *
        VisitVarDeclStmt(VarDeclStmt *vds);

        llvm::Value *
        VisitVarAsgnStmt(VarAsgnStmt *vas);

        llvm::Value *
        VisitFunDeclStmt(FunDeclStmt *fds);

        llvm::Value *
        VisitFunCallStmt(FunCallStmt *fcs);

        llvm::Value *
        VisitRetStmt(RetStmt *rs);

        llvm::Value *
        VisitIfElseStmt(IfElseStmt *ies);
        
        llvm::Value *
        VisitForLoopStmt(ForLoopStmt *fls);

        llvm::Value *
        VisitBreakStmt(BreakStmt *bs);

        llvm::Value *
        VisitContinueStmt(ContinueStmt *cs);

        llvm::Value *
        VisitStructStmt(StructStmt *ss);
        
        llvm::Value *
        VisitFieldAsgnStmt(FieldAsgnStmt *fas);

        llvm::Value *
        VisitImplStmt(ImplStmt *is);

        llvm::Value *
        VisitMethodCallStmt(MethodCallStmt *mcs);

        llvm::Value *
        VisitEchoStmt(EchoStmt *es);

        llvm::Value *
        VisitBinaryExpr(BinaryExpr *be);
        
        llvm::Value *
        VisitUnaryExpr(UnaryExpr *ue);
        
        llvm::Value *
        VisitVarExpr(VarExpr *ve);
        
        llvm::Value *
        VisitLiteralExpr(LiteralExpr *le);

        llvm::Value *
        VisitFunCallExpr(FunCallExpr *fce);

        llvm::Value *
        VisitStructExpr(StructExpr *se);

        llvm::Value *
        VisitFieldAccessExpr(FieldAccessExpr *fae);

        llvm::Value *
        VisitMethodCallExpr(MethodCallExpr *mce);

    private:
        llvm::Type *
        getCommonType(llvm::Type *left, llvm::Type *right);
        
        llvm::Value *
        implicitlyCast(llvm::Value *src, llvm::Type *expectType);

        llvm::SMRange
        getRange(llvm::SMLoc start, int len) const;

        llvm::Type *
        typeToLLVM(ASTType type);

        llvm::Value *
        defaultStructConst(ASTType type);

        std::string
        resolveStructName(Expr *expr);

        //std::string
        //getMangledName(std::string base) const;
    };
}
