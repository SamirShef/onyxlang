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
        llvm::BumpPtrAllocator _allocator;

    public:
        explicit Parser(Lexer &lex, DiagnosticEngine &diag) : _lex(lex), _diag(diag), _lastTok(Token(TkUnknown, "", llvm::SMLoc())),
                                                              _curTok(Token(TkUnknown, "", llvm::SMLoc())), _nextTok(Token(TkUnknown, "", llvm::SMLoc())) {
            consume();
            consume();
        }

        Stmt *
        ParseStmt(bool consumeSemi = true);

    private:
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

        bool
        expect(TokenKind kind);

        bool
        isAssignmentOp(TokenKind kind);

        llvm::SMRange
        getRangeFromTok(Token tok) const;
    };
}
