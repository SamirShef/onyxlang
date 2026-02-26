#pragma once
#include <marble/AST/AST.h>
#include <marble/Basic/DiagnosticEngine.h>
#include <marble/Lexer/Lexer.h>
#include <llvm/Support/Allocator.h>

namespace marble {
    class Parser {
        Lexer &_lex;
        DiagnosticEngine &_diag;
        Token _lastTok;
        Token _curTok;
        Token _nextTok;

    public:
        explicit Parser(Lexer &lex, DiagnosticEngine &diag) : _lex(lex), _diag(diag), _lastTok(Token(TkUnknown, "", llvm::SMLoc())),
                                                              _curTok(Token(TkUnknown, "", llvm::SMLoc())), _nextTok(Token(TkUnknown, "", llvm::SMLoc())) {
            consume();
            consume();
        }

        std::vector<Stmt *>
        ParseAll();

    private:
        template<typename T, typename ...Args>
        T *
        createNode(Args &&... args);
    
        Stmt *
        parseStmt(bool consumeSemi = true);

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
        parseModDeclStmt();

        Argument
        parseArgument();
        
        Expr *
        parsePrefixExpr(bool allowStruct = true);

        Expr *
        parseExpr(int minPrec, bool allowStruct = true);
    
        Expr *
        parseChainExpr(Expr *base);

        Token
        consume();

        ASTType
        consumeType();

        Expr *
        createCompoundAssignmentOp(Token op, Expr *base, Expr *expr);

        bool
        expect(TokenKind kind);

        bool
        isAssignmentOp(TokenKind kind);

        llvm::SMRange
        getRangeFromTok(Token tok) const;
    };
}
