#include <onyx/Parser/Parser.h>
#include <onyx/Parser/Precedence.h>

namespace onyx {
    Stmt *
    Parser::ParseStmt() {
        if (_curTok.Is(TkEof)) {
            return nullptr;
        }
        switch (_curTok.GetKind()) {
            case TkVar:
            case TkConst:
                return parseVarDeclStmt();
            default:
                _diag.Report(_curTok.GetLoc(), ErrExpectedStmt)
                    << getRangeFromTok(_curTok)
                    << _curTok.GetText();
                consume();
                return nullptr;
        }
    }
    
    template <typename T, typename ...Args>
    T *
    Parser::createNode(Args &&... args) {
        void *mem = _allocator.Allocate<T>();
        return new (mem) T(std::forward<Args>(args)...);
    }

    Stmt *
    Parser::parseVarDeclStmt() {
        Token firstTok = _curTok;
        bool isConst = consume().Is(TkConst);
        llvm::StringRef name = _curTok.GetText();
        if (!expect(TkId)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedId)
                << getRangeFromTok(_curTok)
                << _curTok.GetText();
        }

        if (!expect(TkColon)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                << getRangeFromTok(_curTok)
                << ":"                  // expected
                << _curTok.GetText();   // got
        }
        
        ASTType type = consumeType();
        
        Expr *expr = nullptr;
        if (expect(TkEq)) {
            expr = parseExpr(PrecLowest);
        }
        if (!expect(TkSemi)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                << getRangeFromTok(_curTok)
                << ";"                  // expected
                << _curTok.GetText();   // got
        }
        return createNode<VarDeclStmt>(name, isConst, type, expr, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Expr *
    Parser::parsePrefixExpr() {
        switch (_curTok.GetKind()) {
            case TkId: {
                Token nameToken = consume();
                if (_curTok.GetKind() == TkLParen) {
                    // TODO: create logic for calling of functions
                }
                else {
                    return createNode<VarExpr>(nameToken.GetText(), nameToken.GetLoc(), _curTok.GetLoc());
                }
            }
            #define LIT(kind, type_val, field, val) \
                createNode<LiteralExpr>(ASTVal(ASTType(ASTTypeKind::kind, type_val, true), \
                                               ASTValData { .field = (val) }), consume().GetLoc(), _curTok.GetLoc())
            case TkBoolLit:
                return LIT(Bool, "bool", boolVal, _curTok.GetText() == "true");
            case TkCharLit:
                return LIT(Char, "char", charVal, _curTok.GetText()[0]);
            case TkI16Lit:
                return LIT(I16, "i16", i16Val, static_cast<short>(std::stoi(_curTok.GetText().data())));
            case TkI32Lit:
                return LIT(I32, "i32", i32Val, std::stoi(_curTok.GetText().data()));
            case TkI64Lit:
                return LIT(I64, "i64", i64Val, std::stol(_curTok.GetText().data()));
            case TkF32Lit:
                return LIT(F32, "f32", f32Val, std::stof(_curTok.GetText().data()));
            case TkF64Lit:
                return LIT(F64, "f64", f64Val, std::stod(_curTok.GetText().data()));
            #undef LIT
            case TkMinus:
            case TkBang: {
                Token op = consume();
                return createNode<UnaryExpr>(parseExpr(PrecLowest), op, op.GetLoc(), _curTok.GetLoc());
            }
            case TkLParen: {
                Token lparen = consume();
                Expr *expr = parseExpr(PrecLowest);
                if (!expect(TkRParen)) {
                    _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                        << getRangeFromTok(_curTok)
                        << ")"                  // expected
                        << _curTok.GetText();   // got
                }
                expr->SetStartLoc(lparen.GetLoc());
                expr->SetEndLoc(_curTok.GetLoc());
                return expr;
            }
            default:
                _diag.Report(_curTok.GetLoc(), ErrExpectedExpr)
                    << getRangeFromTok(_curTok)
                    << _curTok.GetText();
                consume();
                return nullptr;
        }
    }

    Expr *
    Parser::parseExpr(int minPrec) {
        Expr *lhs = parsePrefixExpr();
        if (!lhs) {
            return nullptr;
        }

        while (minPrec < GetTokPrecedence(_curTok.GetKind())) {
            Token op = _curTok;
            int prec = GetTokPrecedence(_curTok.GetKind());
            consume();

            Expr *rhs = parseExpr(prec);
            lhs = createNode<BinaryExpr>(lhs, rhs, op, lhs->GetStartLoc(), _curTok.GetLoc());
        }

        return lhs;
    }
    
    Token
    Parser::consume() {
        Token tok = _curTok;
        _curTok = _nextTok;
        _nextTok = _lex.NextToken();
        return tok;
    }

    ASTType
    Parser::consumeType() {
        bool isConst = expect(TkConst);
        Token type = consume();
        switch (type.GetKind()) {
            #define TYPE(kind, type_val) ASTType(ASTTypeKind::kind, type_val, isConst)
            case TkBool:
                return TYPE(Bool, "bool");
            case TkChar:
                return TYPE(Char, "char");
            case TkI16:
                return TYPE(I16, "i16");
            case TkI32:
                return TYPE(I32, "i32");
            case TkI64:
                return TYPE(I64, "i64");
            case TkF32:
                return TYPE(F32, "f32");
            case TkF64:
                return TYPE(F64, "f64");
            default:
                _diag.Report(type.GetLoc(), ErrExpectedType)
                    << getRangeFromTok(type)
                    << type.GetText();
                return TYPE(I32, "i32");
            #undef TYPE
        }
    }

    bool
    Parser::expect(TokenKind kind) {
        if (_curTok.Is(kind)) {
            consume();
            return true;
        }
        return false;
    }

    llvm::SMRange
    Parser::getRangeFromTok(Token tok) const {
        return llvm::SMRange(tok.GetLoc(), llvm::SMLoc::getFromPointer(tok.GetLoc().getPointer() + tok.GetText().size()));
    }
}