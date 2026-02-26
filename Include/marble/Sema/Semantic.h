#pragma once
#include <marble/Basic/Module.h>
#include <marble/AST/AST.h>
#include <marble/AST/Visitor.h>
#include <marble/Basic/DiagnosticEngine.h>
#include <marble/Parser/Parser.h>
#include <stack>
#include <unordered_map>

namespace marble {
    class SemanticAnalyzer : public ASTVisitor<SemanticAnalyzer, std::optional<ASTVal>> {
        DiagnosticEngine &_diag;
        Module *_curMod;

        std::stack<std::unordered_map<std::string, Variable>> _vars;

        std::stack<ASTType> _funRetsTypes;

        int _loopDeth = 0;
        
    public:
        explicit SemanticAnalyzer(DiagnosticEngine &diag, Module *root) : _diag(diag), _curMod(root) {
            _vars.push({});
        }

        void
        AnalyzeModule(Module *mod, bool isRoot = false);

        void
        DeclareInModule(Module *mod);
        
        /*
        void
        DeclareFunctions(std::vector<Stmt *> &ast);
        */

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
        VisitModDeclStmt(ModDeclStmt *mds);

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
        ASTType
        resolveType(ASTType &type);

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
    };
}
