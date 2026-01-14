#include <onyx/Parser/Parser.h>
#include <onyx/Parser/Precedence.h>

namespace onyx {
    Stmt *
    Parser::ParseStmt(bool consumeSemi) {
        if (_curTok.Is(TkEof)) {
            return nullptr;
        }
        switch (_curTok.GetKind()) {
            case TkVar:
            case TkConst: {
                Stmt *stmt = parseVarDeclStmt();
                if (consumeSemi && !expect(TkSemi)) {
                    _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                        << getRangeFromTok(_curTok)
                        << ";"                  // expected
                        << _curTok.GetText();   // got
                }
                return stmt;
            }
            case TkFun: {
                return parseFunDeclStmt();
            }
            case TkRet: {
                Stmt *stmt = parseRetStmt();
                if (consumeSemi && !expect(TkSemi)) {
                    _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                        << getRangeFromTok(_curTok)
                        << ";"                  // expected
                        << _curTok.GetText();   // got
                }
                return stmt;
            }
            case TkId: {
                if (_nextTok.Is(TkLParen)) {
                    Stmt *stmt = parseFunCallStmt();
                    if (consumeSemi && !expect(TkSemi)) {
                        _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                            << getRangeFromTok(_curTok)
                            << ";"                  // expected
                            << _curTok.GetText();   // got
                    }
                    return stmt;
                }
                Stmt *stmt = parseVarAsgn();
                if (consumeSemi && !expect(TkSemi)) {
                    _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                        << getRangeFromTok(_curTok)
                        << ";"                  // expected
                        << _curTok.GetText();   // got
                }
                return stmt;
            }
            case TkIf:
                return parseIfElseStmt();
            case TkFor:
                return parseForLoopStmt();
            case TkBreak: {
                Token firstTok = consume();
                if (consumeSemi && !expect(TkSemi)) {
                    _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                        << getRangeFromTok(_curTok)
                        << ";"                  // expected
                        << _curTok.GetText();   // got
                }
                return createNode<BreakStmt>(firstTok.GetLoc(), _curTok.GetLoc());
            }
            case TkContinue: {
                Token firstTok = consume();
                if (consumeSemi && !expect(TkSemi)) {
                    _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                        << getRangeFromTok(_curTok)
                        << ";"                  // expected
                        << _curTok.GetText();   // got
                }
                return createNode<ContinueStmt>(firstTok.GetLoc(), _curTok.GetLoc());
            }
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
        return createNode<VarDeclStmt>(name, isConst, type, expr, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseVarAsgn() {
        Token nameToken = consume();
        if (!isAssignmentOp(_curTok.GetKind())) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                << getRangeFromTok(_curTok)
                << "=` or `+=` or `-=` or `*=` or `/=` or `%="
                << _curTok.GetText().str();
        }
        Token op = consume();
        Expr *expr = parseExpr(PrecLowest);
        if (op.GetKind() != TkEq && isAssignmentOp(op.GetKind())) {
            expr = createCompoundAssignmentOp(op, createNode<VarExpr>(nameToken.GetText(), nameToken.GetLoc(), op.GetLoc()), expr);
        }
        return createNode<VarAsgnStmt>(nameToken.GetText(), expr, nameToken.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseFunDeclStmt() {
        Token firstTok = consume();
        llvm::StringRef name = _curTok.GetText();
        if (!expect(TkId)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedId)
                << getRangeFromTok(_curTok)
                << _curTok.GetText();
        }
        if (!expect(TkLParen)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                << getRangeFromTok(_curTok)
                << "("                  // expected
                << _curTok.GetText();   // got
        }
        std::vector<Argument> args;
        while (!expect(TkRParen)) {
            args.push_back(parseArgument());
            if (!_curTok.Is(TkRParen)) {
                if (!expect(TkComma)) {
                    _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                        << getRangeFromTok(_curTok)
                        << ","                  // expected
                        << _curTok.GetText();   // got
                }
            }
        }

        ASTType retType = ASTType(ASTTypeKind::Noth, "noth", false);
        if (expect(TkColon)) {
            retType = consumeType();
        }

        if (!expect(TkLBrace)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                << getRangeFromTok(_curTok)
                << "{"                  // expected
                << _curTok.GetText();   // got
        }
        std::vector<Stmt *> block;
        while (!expect(TkRBrace)) {
            block.push_back(ParseStmt());
        }
        return createNode<FunDeclStmt>(name, retType, args, block, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseFunCallStmt() {
        Token nameToken = consume();
        consume();
        std::vector<Expr *> args;
        while (!expect(TkRParen)) {
            args.push_back(parseExpr(PrecLowest));
            if (!_curTok.Is(TkRParen)) {
                if (!expect(TkComma)) {
                    _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                        << getRangeFromTok(_curTok)
                        << ","                  // expected
                        << _curTok.GetText();   // got
                }
            }
        }
        return createNode<FunCallStmt>(nameToken.GetText(), args, nameToken.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseRetStmt() {
        Token firstTok = consume();
        Expr *expr = nullptr;
        if (!_curTok.Is(TkSemi)) {
            expr = parseExpr(PrecLowest);
        }
        return createNode<RetStmt>(expr, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseIfElseStmt() {
        Token firstTok = consume();
        Expr *cond = parseExpr(PrecLowest);
        std::vector<Stmt *> thenBranch;
        if (!expect(TkLBrace)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                << getRangeFromTok(_curTok)
                << "{"                  // expected
                << _curTok.GetText();   // got
        }
        else {
            while (!expect(TkRBrace)) {
                thenBranch.push_back(ParseStmt());
            }
        }
        std::vector<Stmt *> elseBranch;
        if (expect(TkElse)) {
            if (expect(TkLBrace)) {
                while (!expect(TkRBrace)) {
                    elseBranch.push_back(ParseStmt());
                }
            }
            else {
                elseBranch.push_back(ParseStmt());
            }
        }
        return createNode<IfElseStmt>(cond, thenBranch, elseBranch, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseForLoopStmt() {
        Token firstTok = consume();
        Stmt *indexator = nullptr;
        if (_curTok.Is(TkVar) || _curTok.Is(TkId) && isAssignmentOp(_nextTok.GetKind())) {
            indexator = ParseStmt(false);
            if (!expect(TkComma)) {
                _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                    << getRangeFromTok(_curTok)
                    << ","                  // expected
                    << _curTok.GetText();   // got
            }
        }
        Expr *cond = parseExpr(PrecLowest);
        Stmt *iteration = nullptr;
        if (!_curTok.Is(TkLBrace)) {
            if (!expect(TkComma)) {
                _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                    << getRangeFromTok(_curTok)
                    << ","                  // expected
                    << _curTok.GetText();   // got
            }
            iteration = ParseStmt(false);
        }
        std::vector<Stmt *> block;
        if (!expect(TkLBrace)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                << getRangeFromTok(_curTok)
                << "{"                  // expected
                << _curTok.GetText();   // got
        }
        else {
            while (!expect(TkRBrace)) {
                block.push_back(ParseStmt());
            }
        }
        return createNode<ForLoopStmt>(indexator, cond, iteration, block, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Argument
    Parser::parseArgument() {
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
        return Argument(name, type);
    }

    Expr *
    Parser::parsePrefixExpr() {
        switch (_curTok.GetKind()) {
            case TkId: {
                Token nameToken = consume();
                if (_curTok.GetKind() == TkLParen) {
                    consume();
                    std::vector<Expr *> args;
                    while (!expect(TkRParen)) {
                        args.push_back(parseExpr(PrecLowest));
                        if (!_curTok.Is(TkRParen)) {
                            if (!expect(TkComma)) {
                                _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                                    << getRangeFromTok(_curTok)
                                    << ","                  // expected
                                    << _curTok.GetText();   // got
                            }
                        }
                    }
                    return createNode<FunCallExpr>(nameToken.GetText(), args, nameToken.GetLoc(), _curTok.GetLoc());
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
            Token op = consume();
            int prec = GetTokPrecedence(op.GetKind());

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

    Expr *
    Parser::createCompoundAssignmentOp(Token op, Expr *base, Expr *expr) {
        switch (op.GetKind()) {
            #define OP(kind, val) createNode<BinaryExpr>(base, expr, Token(kind, val, op.GetLoc()), expr->GetStartLoc(), expr->GetEndLoc())
            case TkPlusEq:
                return OP(TkPlus, "+");
            case TkMinusEq:
                return OP(TkMinus, "-");
            case TkStarEq:
                return OP(TkStar, "*");
            case TkSlashEq:
                return OP(TkSlash, "/");
            case TkPercentEq:
                return OP(TkPercent, "%");
            default:
                return nullptr;
            #undef OP
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

    bool
    Parser::isAssignmentOp(TokenKind kind) {
        return kind >= TkPlusEq && kind <= TkPercentEq || kind == TkEq;
    }

    llvm::SMRange
    Parser::getRangeFromTok(Token tok) const {
        return llvm::SMRange(tok.GetLoc(), llvm::SMLoc::getFromPointer(tok.GetLoc().getPointer() + tok.GetText().size()));
    }
}