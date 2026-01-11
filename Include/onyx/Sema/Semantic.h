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

    public:
        explicit SemanticAnalyzer(DiagnosticEngine &diag) : _diag(diag) {
            _vars.push({});
        }
        
        std::optional<ASTVal>
        visitVarDeclStmt(VarDeclStmt *vds);

        std::optional<ASTVal>
        visitFunDeclStmt(FunDeclStmt *fds);

        std::optional<ASTVal>
        visitFunCallStmt(FunCallStmt *fcs);

        std::optional<ASTVal>
        visitRetStmt(RetStmt *rs);
        
        std::optional<ASTVal>
        visitBinaryExpr(BinaryExpr *be);
        
        std::optional<ASTVal>
        visitUnaryExpr(UnaryExpr *ue);
        
        std::optional<ASTVal>
        visitVarExpr(VarExpr *ve);
        
        std::optional<ASTVal>
        visitLiteralExpr(LiteralExpr *le);

        std::optional<ASTVal>
        visitFunCallExpr(FunCallExpr *fce);

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