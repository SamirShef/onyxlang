#pragma once
#include <onyx/AST/AST.h>
#include <onyx/Basic/DiagnosticEngine.h>
#include <onyx/Lexer/Lexer.h>
#include <llvm/Support/Allocator.h>

namespace onyx {
    class Parser {
        Lexer &_lex;
        DiagnosticEngine &_diag;
        Token _curTok;
        Token _nextTok;
        llvm::BumpPtrAllocator _allocator;

    public:
        explicit Parser(Lexer &lex, DiagnosticEngine &diag) : _lex(lex), _diag(diag), _curTok(Token(TkUnknown, "", llvm::SMLoc())),
                                                              _nextTok(Token(TkUnknown, "", llvm::SMLoc())) {
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
        parseVarDeclStmt();

        Stmt *
        parseVarAsgn();

        Stmt *
        parseFunDeclStmt();

        Stmt *
        parseFunCallStmt();
        
        Stmt *
        parseRetStmt();

        Stmt *
        parseIfElseStmt();

        Stmt *
        parseForLoopStmt();

        Stmt *
        parseStructStmt();

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
