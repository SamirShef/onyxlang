#pragma once
#include <marble/Basic/Module.h>
#include <marble/AST/Visitor.h>
#include <marble/Basic/DiagnosticEngine.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <stack>
#include <string>

namespace marble {
    class CodeGen : public ASTVisitor<CodeGen, llvm::Value *> {
        llvm::SourceMgr &_srcMgr;
        DiagnosticEngine &_diag;
        llvm::LLVMContext _context;
        std::unique_ptr<llvm::Module> _module;
        llvm::IRBuilder<> _builder;
        Module *_curMod = nullptr;
        std::string _parentDir;

        std::stack<std::unordered_map<std::string, std::tuple<llvm::Value *, llvm::Type *, ASTType>>> _vars;

        std::unordered_map<std::string, llvm::Function *> _functions;
        std::unordered_map<std::string, std::vector<ASTType>> _funArgsTypes;
        std::stack<llvm::Type *> _funRetsTypes;

        std::stack<std::pair<llvm::BasicBlock *, llvm::BasicBlock *>> _loopDeth;    // first for break, second for continue

        std::unordered_map<std::string, llvm::Constant *> _strPool;

        struct Field {
            std::string Name;
            llvm::Type *Type;
            marble::ASTType ASTType;
            Expr *DefaultExpr = nullptr;
            bool ManualInitialized;
            long Index;
        };

        struct Method {
            std::string Name;
            llvm::Type *RetType;
            std::vector<llvm::Type *> Args;
        };

        struct Trait {
            std::string Name;
            std::string MangledName;
            std::vector<std::pair<std::string, Method>> Methods; 
        };
        std::unordered_map<std::string, Trait> _traits;

        struct Struct {
            std::string Name;
            std::string MangledName;
            llvm::StructType *Type;
            std::unordered_map<std::string, Field> Fields;
            std::unordered_map<std::string, Trait> TraitsImplements;
        };
        std::unordered_map<std::string, Struct> _structs;

    public:
        explicit CodeGen(std::string parentDir, std::string fileName, llvm::SourceMgr &mgr, DiagnosticEngine &diag)
                       : _srcMgr(mgr), _diag(diag), _context(), _builder(_context), _module(std::make_unique<llvm::Module>(fileName, _context)), _parentDir(parentDir) {
            _vars.push({});
        }

        std::unique_ptr<llvm::Module>
        GetModule() {
            return std::move(_module);
        }

        void
        DeclareMod(Module *mod);

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
        VisitTraitDeclStmt(TraitDeclStmt *tds);

        llvm::Value *
        VisitEchoStmt(EchoStmt *es);

        llvm::Value *
        VisitDelStmt(DelStmt *ds);

        llvm::Value *
        VisitImportStmt(ImportStmt *is);

        llvm::Value *
        VisitModDeclStmt(ModDeclStmt *mds);

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

        llvm::Value *
        VisitNilExpr(NilExpr *ne);

        llvm::Value *
        VisitDerefExpr(DerefExpr *de);

        llvm::Value *
        VisitRefExpr(RefExpr *re);

        llvm::Value *
        VisitNewExpr(NewExpr *ne);

    private:
        void
        declareRuntimeFunctions();

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

        void
        createCheckForNil(llvm::Value *ptr, llvm::SMLoc loc);

        llvm::Value *
        getOrCreateVTable(const std::string &structName, const std::string &traitName);

        llvm::Value *
        castToTrait(llvm::Value *src, llvm::Type *traitType, const std::string &structName);

        llvm::Constant *
        getOrCreateGlobalString(std::string val, std::string name = "");

        std::string
        getMangledName(std::string name, char sep = '.', Module *mod = nullptr);

        std::string
        getMangledName(ASTType type, char sep = '.');
    };
}
