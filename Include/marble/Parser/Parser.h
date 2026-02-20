#pragma once
#include <marble/AST/AST.h>
#include <marble/Basic/DiagnosticEngine.h>
#include <marble/Lexer/Lexer.h>
#include <llvm/Support/Allocator.h>

namespace marble {
    class Module;
    class ModuleManager;

    class Parser {
        Lexer &_lex;
        DiagnosticEngine &_diag;
        llvm::SourceMgr &_srcMgr;
        ModuleManager &_modManager;
        Token _lastTok;
        Token _curTok;
        Token _nextTok;

    public:
        explicit Parser(Lexer &lex, DiagnosticEngine &diag, llvm::SourceMgr &srcMgr, ModuleManager &mm)
                      : _lex(lex), _diag(diag), _srcMgr(srcMgr), _modManager(mm), _lastTok(Token(TkUnknown, "", llvm::SMLoc())),
                        _curTok(Token(TkUnknown, "", llvm::SMLoc())), _nextTok(Token(TkUnknown, "", llvm::SMLoc())) {
            consume();
            consume();
        }

        std::vector<Stmt *>
        ParseAll();

    private:
        Stmt *
        ParseStmt(bool consumeSemi = true);

        template<typename T, typename ...Args>
        T *
        createNode(Args &&... args);
    
        Stmt *
        parseIdStartStmt();

        Stmt *
        parseVarDeclStmt();

        Stmt *
        parseVarAsgn(unsigned char derefDepth = 0);

        Stmt *
        parseFieldAsgnStmt(Expr *base);

        Stmt *
        parseFunDeclStmt();

        Stmt *
        parseRetStmt();

        Stmt *
        parseIfElseStmt();

        Stmt *
        parseForLoopStmt();

        Stmt *
        parseStructStmt();

        Stmt *
        parseImplStmt();

        Stmt *
        parseTraitDeclStmt();

        Stmt *
        parseDelStmt();

        Stmt *
        parseImportStmt();

        Stmt *
        parseModuleDeclStmt();

        Argument
        parseArgument();
        
        Expr *
        parsePrefixExpr();

        Expr *
        parseExpr(int minPrec);
    
        Expr *
        parseChainExpr(Expr *base);

        Token
        consume();

        ASTType
        consumeType();

        Expr *
        createCompoundAssignmentOp(Token op, Expr *base, Expr *expr);

        void
        importModuleHandler(std::string path, bool isLocalImport, llvm::SMLoc startLoc);

        void
        registerTypes(Module *mod);

        bool
        expect(TokenKind kind);

        bool
        isAssignmentOp(TokenKind kind);

        llvm::SMRange
        getRangeFromTok(Token tok) const;
    };
}
