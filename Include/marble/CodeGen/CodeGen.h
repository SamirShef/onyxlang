#pragma once
#include <marble/Basic/ModuleManager.h>
#include <marble/AST/Visitor.h>
#include <marble/Basic/DiagnosticEngine.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <stack>

namespace marble {
    class CodeGen : public ASTVisitor<CodeGen, llvm::Value *> {
        llvm::SourceMgr &_srcMgr;
        llvm::LLVMContext _context;
        Module *_module;
        llvm::IRBuilder<> _builder;

        std::stack<std::unordered_map<std::string, std::tuple<llvm::Value *, llvm::Type *, ASTType>>> _vars;

        std::unordered_map<std::string, llvm::Function *> _functions;
        std::unordered_map<std::string, std::vector<ASTType>> _funArgsTypes;
        std::stack<llvm::Type *> _funRetsTypes;

        std::stack<std::pair<llvm::BasicBlock *, llvm::BasicBlock *>> _loopDeth;    // first for break, second for continue

        std::vector<std::string> _modulesPath;
        Module *_currentMod = nullptr;
        Module *_rootMod = nullptr;

        struct Field {
            std::string Name;
            llvm::Type *Type;
            marble::ASTType ASTType;
            llvm::Value *Val;
            bool ManualInitialized;
            bool IsStatic;
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
        explicit CodeGen(Module *mod, llvm::SourceMgr &srcMgr) : _srcMgr(srcMgr), _context(), _builder(_context),
                                                                          _module(mod) {
            _module->Mod = new llvm::Module(mod->GetName(), _context);
            _vars.push({});
        }

        llvm::Module *
        GetLLVMModule() {
            return _module->Mod;
        }

        void
        DeclareMod(Module *mod);

        void
        DeclareStatements(std::vector<Stmt *> ast);

        void
        DeclareRuntimeFunctions();

        void
        GenerateBodies(Module *mod);

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
        VisitModuleDeclStmt(ModuleDeclStmt *mds);

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

        llvm::Function *
        getFunction(std::string name);

        std::string
        getMangledName(const std::vector<std::string> &path, const std::string &name) const;

        std::string
        getCurrentMangled(const std::string &name) const;

        std::vector<std::string>
        splitPath(const std::string &path);

        std::string
        getMangledForPath(const std::string &path);

        std::string
        getModulePathFromExpr(Expr *expr);

        std::string
        resolveFullTypeName(const ASTType& type);
    };
}
