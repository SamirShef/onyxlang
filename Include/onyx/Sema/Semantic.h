#pragma once
#include <onyx/AST/AST.h>
#include <onyx/AST/Visitor.h>
#include <onyx/Basic/DiagnosticEngine.h>
#include <onyx/Parser/Parser.h>
#include <stack>
#include <unordered_map>

namespace onyx {
    class SemanticAnalyzer : public ASTVisitor<SemanticAnalyzer, std::optional<ASTVal>> {
        DiagnosticEngine &_diag;
        
        struct Variable {
            const llvm::StringRef Name;
            const ASTType Type;
            std::optional<ASTVal> Val;
            const bool IsConst;
        };
        std::stack<std::unordered_map<std::string, Variable>> _vars;

        struct Function {
            const llvm::StringRef Name;
            const ASTType RetType;
            std::vector<Argument> Args;
            std::vector<Stmt *> Body;
        };
        std::unordered_map<std::string, Function> functions;
        std::stack<ASTType> funRetsTypes;

        int _loopDeth = 0;
        
    public:
        explicit SemanticAnalyzer(DiagnosticEngine &diag) : _diag(diag) {
            _vars.push({});
        }
        
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
        VisitBinaryExpr(BinaryExpr *be);
        
        std::optional<ASTVal>
        VisitUnaryExpr(UnaryExpr *ue);
        
        std::optional<ASTVal>
        VisitVarExpr(VarExpr *ve);
        
        std::optional<ASTVal>
        VisitLiteralExpr(LiteralExpr *le);

        std::optional<ASTVal>
        VisitFunCallExpr(FunCallExpr *fce);

    private:
        llvm::SMRange
        getRange(llvm::SMLoc start, int len) const;

        void
        checkBinaryExpr(BinaryExpr *be);

        bool
        variableExists(std::string name) const;

        bool
        canImplicitlyCast(ASTVal src, ASTType expectType) const;
        
        ASTVal
        implicitlyCast(ASTVal src, ASTType expectType, llvm::SMLoc startLoc, llvm::SMLoc endLoc) const;
    };
}