#pragma once
#include <onyx/ast/AST.h>
#include <onyx/basic/DiagnosticEngine.h>
#include <onyx/lexer/Lexer.h>
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
        ParseStmt();

    private:
        template<typename T, typename ...Args>
        T *
        createNode(Args &&... args);
    
        Stmt *
        parseVarDeclStmt();

        Expr *
        parsePrefixExpr();

        Expr *
        parseExpr(int minPrec);
    
        Token
        consume();

        ASTType
        consumeType();

        bool
        expect(TokenKind kind);

        llvm::SMRange
        getRangeFromTok(Token tok) const;
    };
}