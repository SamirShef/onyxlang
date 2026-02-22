#pragma once
#include <marble/Basic/ModuleManager.h>
#include <marble/AST/AST.h>
#include <marble/AST/Visitor.h>
#include <marble/Basic/DiagnosticEngine.h>
#include <marble/Parser/Parser.h>
#include <stack>
#include <unordered_map>

namespace marble {
    class SemanticAnalyzer : public ASTVisitor<SemanticAnalyzer, std::optional<ASTVal>> {
        DiagnosticEngine &_diag;
        llvm::SourceMgr &_srcMgr;
        ModuleManager &_modManager;
        Module *_rootMod = nullptr;
        Module *_currentMod = nullptr;
        const std::string &_libsPath;
        
        std::stack<std::unordered_map<std::string, Variable>> _vars;

        std::stack<ASTType> _funRetsTypes;
        int _loopDeth = 0;
        
    public:
        explicit SemanticAnalyzer(DiagnosticEngine &diag, llvm::SourceMgr &srcMgr, const std::string &libsPath, ModuleManager &mm)
                                : _diag(diag), _srcMgr(srcMgr), _libsPath(libsPath), _modManager(mm) {
            _vars.push({});
        }
        
        void
        Analyze(Module *mod);

        std::optional<ASTVal>
        VisitVarDeclStmt(VarDeclStmt *vds);

        std::optional<ASTVal>
        VisitVarAsgnStmt(VarAsgnStmt *vas);

        std::optional<ASTVal>
        VisitFunDeclStmt(FunDeclStmt *fds);

        std::optional<ASTVal>
        VisitFunCallStmt(FunCallStmt *fcs);

        std::optional<ASTVal>
        VisitRetStmt(RetStmt *rs);

        std::optional<ASTVal>
        VisitIfElseStmt(IfElseStmt *ies);

        std::optional<ASTVal>
        VisitForLoopStmt(ForLoopStmt *fls);

        std::optional<ASTVal>
        VisitBreakStmt(BreakStmt *bs);

        std::optional<ASTVal>
        VisitContinueStmt(ContinueStmt *cs);

        std::optional<ASTVal>
        VisitStructStmt(StructStmt *ss);
        
        std::optional<ASTVal>
        VisitFieldAsgnStmt(FieldAsgnStmt *fas);

        std::optional<ASTVal>
        VisitImplStmt(ImplStmt *is);

        std::optional<ASTVal>
        VisitMethodCallStmt(MethodCallStmt *mcs);

        std::optional<ASTVal>
        VisitTraitDeclStmt(TraitDeclStmt *tds);

        std::optional<ASTVal>
        VisitEchoStmt(EchoStmt *es);

        std::optional<ASTVal>
        VisitDelStmt(DelStmt *ds);

        std::optional<ASTVal>
        VisitImportStmt(ImportStmt *is);

        std::optional<ASTVal>
        VisitModuleDeclStmt(ModuleDeclStmt *mds);

        std::optional<ASTVal>
        VisitBinaryExpr(BinaryExpr *be);
        
        std::optional<ASTVal>
        VisitUnaryExpr(UnaryExpr *ue);
        
        std::optional<ASTVal>
        VisitVarExpr(VarExpr *ve);
        
        std::optional<ASTVal>
        VisitLiteralExpr(LiteralExpr *le);

        std::optional<ASTVal>
        VisitFunCallExpr(FunCallExpr *fce);

        std::optional<ASTVal>
        VisitStructExpr(StructExpr *se);

        std::optional<ASTVal>
        VisitFieldAccessExpr(FieldAccessExpr *fae);

        std::optional<ASTVal>
        VisitMethodCallExpr(MethodCallExpr *mce);

        std::optional<ASTVal>
        VisitNilExpr(NilExpr *ne);

        std::optional<ASTVal>
        VisitDerefExpr(DerefExpr *de);

        std::optional<ASTVal>
        VisitRefExpr(RefExpr *re);

        std::optional<ASTVal>
        VisitNewExpr(NewExpr *ne);

    private:
        void
        resolveTypeInStatement(Stmt *stmt, Module *mod);

        void
        discover(Module *mod);

        bool
        inRootMod(const Module *mod);

        Variable *
        findVar(std::string name);

        Function *
        findFunction(std::string name);
        
        Struct *
        findStruct(std::string name);

        Trait *
        findTrait(std::string name);

        llvm::SMRange
        getRange(llvm::SMLoc start, int len) const;

        void
        checkBinaryExpr(BinaryExpr *be);

        bool
        variableExists(std::string name) const;

        bool
        canImplicitlyCast(ASTVal src, ASTType expectType);
        
        ASTVal
        implicitlyCast(ASTVal src, ASTType expectType, llvm::SMLoc startLoc, llvm::SMLoc endLoc);

        std::vector<std::string>
        splitPath(const std::string &path);

        Struct *
        findStructByPath(const std::string &path, Module *contextMod = nullptr);
        
        Trait *
        findTraitByPath(const std::string &path, Module *contextMod = nullptr);
        
        ASTType
        resolveType(ASTType type, Module *contextMod);

        Module *
        createModule(Module *base, std::string name, std::string fullPath, AccessModifier access, std::vector<Stmt *> ast);
    };
}
