#include <iostream>
#include <llvm/Support/Path.h>
#include <marble/Parser/Parser.h>
#include <marble/Parser/Precedence.h>

static marble::AccessModifier access;

extern std::unordered_map<std::string, marble::ASTType> types;
extern std::string libsPath;

namespace marble {
    std::vector<Stmt *>
    Parser::ParseAll() {
        std::vector<Stmt *> ast;
        while (!_curTok.Is(TkEof)) {
            if (Stmt *s = ParseStmt()) {
                ast.push_back(s);
            }
        }
        return ast;
    }

    Stmt *
    Parser::ParseStmt(bool consumeSemi) {
        if (_curTok.Is(TkEof)) {
            return nullptr;
        }
        access = AccessPriv;
        if (expect(TkPub)) {
            access = AccessPub;
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
                Stmt *stmt = parseIdStartStmt();
                if (consumeSemi && !expect(TkSemi)) {
                    _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                        << getRangeFromTok(_curTok)
                        << ";"                  // expected
                        << _curTok.GetText();   // got
                }
                return stmt;
            }
            case TkStar: {
                unsigned char derefDepth = 0;
                Token star = _curTok;
                for (; expect(TkStar); ++derefDepth);
                consume();
                VarAsgnStmt *vas = llvm::cast<VarAsgnStmt>(parseVarAsgn(derefDepth));
                vas->SetDerefDepth(derefDepth);
                vas->SetStartLoc(star.GetLoc());
                if (consumeSemi && !expect(TkSemi)) {
                    _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                        << getRangeFromTok(_curTok)
                        << ";"                  // expected
                        << _curTok.GetText();   // got
                }
                return vas;
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
                return createNode<BreakStmt>(access, firstTok.GetLoc(), _curTok.GetLoc());
            }
            case TkContinue: {
                Token firstTok = consume();
                if (consumeSemi && !expect(TkSemi)) {
                    _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                        << getRangeFromTok(_curTok)
                        << ";"                  // expected
                        << _curTok.GetText();   // got
                }
                return createNode<ContinueStmt>(access, firstTok.GetLoc(), _curTok.GetLoc());
            }
            case TkStruct: {
                return parseStructStmt();
            }
            case TkImpl: {
                return parseImplStmt();
            }
            case TkEcho: {
                Token first = consume();
                Expr *expr = parseExpr(PrecLowest);
                if (consumeSemi && !expect(TkSemi)) {
                    _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                        << getRangeFromTok(_curTok)
                        << ";"                  // expected
                        << _curTok.GetText();   // got
                }
                return createNode<EchoStmt>(expr, access, first.GetLoc(), _curTok.GetLoc());
            }
            case TkTrait: {
                return parseTraitDeclStmt();
            }
            case TkDel: {
                Stmt *stmt = parseDelStmt();
                if (consumeSemi && !expect(TkSemi)) {
                    _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                        << getRangeFromTok(_curTok)
                        << ";"                  // expected
                        << _curTok.GetText();   // got
                }
                return stmt;
            }
            case TkImport: {
                Stmt *stmt = parseImportStmt();
                if (consumeSemi && !expect(TkSemi)) {
                    _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                        << getRangeFromTok(_curTok)
                        << ";"                  // expected
                        << _curTok.GetText();   // got
                }
                return stmt;
            }
            case TkMod: {
                return parseModuleDeclStmt();
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
        return new T(std::forward<Args>(args)...);
    }

    Stmt *
    Parser::parseIdStartStmt() {
        Expr *expr = parsePrefixExpr();
        switch (expr->GetKind()) {
            case NkVarExpr:
                return parseVarAsgn();
            case NkFunCallExpr: {
                FunCallExpr *fce = llvm::cast<FunCallExpr>(expr);
                return createNode<FunCallStmt>(fce->GetName(), fce->GetArgs(), access, fce->GetStartLoc(), _curTok.GetLoc());
            }
            case NkFieldAccessExpr:
                return parseFieldAsgnStmt(llvm::dyn_cast<FieldAccessExpr>(expr)->GetObject());
            case NkMethodCallExpr: {
                MethodCallExpr *mce = llvm::cast<MethodCallExpr>(expr);
                return createNode<MethodCallStmt>(mce->GetObject(), mce->GetName(), mce->GetArgs(), access, mce->GetStartLoc(), _curTok.GetLoc());
            }
            default: {}
        }
        return nullptr;
    }

    Stmt *
    Parser::parseVarDeclStmt() {
        Token firstTok = _curTok;
        bool isConst = consume().Is(TkConst);
        std::string name = _curTok.GetText();
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
        return createNode<VarDeclStmt>(name, isConst, type, expr, access, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseVarAsgn(unsigned char derefDepth) {
        Token nameToken = _lastTok;
        if (!isAssignmentOp(_curTok.GetKind())) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                << getRangeFromTok(_curTok)
                << "=` or `+=` or `-=` or `*=` or `/=` or `%="
                << _curTok.GetText();
        }
        Token op = consume();
        Expr *expr = parseExpr(PrecLowest);
        Expr *base = createNode<VarExpr>(nameToken.GetText(), llvm::SMLoc::getFromPointer(nameToken.GetLoc().getPointer() - derefDepth), op.GetLoc());
        for (; derefDepth > 0; base = createNode<DerefExpr>(base, llvm::SMLoc::getFromPointer(base->GetStartLoc().getPointer() - derefDepth), base->GetEndLoc()), --derefDepth);
        if (op.GetKind() != TkEq && isAssignmentOp(op.GetKind())) {
            expr = createCompoundAssignmentOp(op, base, expr);
        }
        return createNode<VarAsgnStmt>(nameToken.GetText(), expr, access, nameToken.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseFieldAsgnStmt(Expr *base) {
        Token nameToken = _lastTok;
        if (!isAssignmentOp(_curTok.GetKind())) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                << getRangeFromTok(_curTok)
                << "=` or `+=` or `-=` or `*=` or `/=` or `%="
                << _curTok.GetText();
        }
        Token op = consume();
        Expr *expr = parseExpr(PrecLowest);
        if (op.GetKind() != TkEq && isAssignmentOp(op.GetKind())) {
            expr = createCompoundAssignmentOp(op, createNode<FieldAccessExpr>(base, nameToken.GetText(), nameToken.GetLoc(), op.GetLoc()), expr);
        }
        return createNode<FieldAsgnStmt>(base, nameToken.GetText(), expr, access, base->GetStartLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseFunDeclStmt() {
        Token firstTok = consume();
        std::string name = _curTok.GetText();
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

        ASTType retType = ASTType(ASTTypeKind::Noth, "noth", false, 0);
        if (expect(TkColon)) {
            retType = consumeType();
        }

        if (expect(TkSemi)) {
            return createNode<FunDeclStmt>(name, retType, args, std::vector<Stmt *> {}, true, access, firstTok.GetLoc(), _curTok.GetLoc());
        }
        if (!expect(TkLBrace)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                << getRangeFromTok(_curTok)
                << "{ or ';"            // expected
                << _curTok.GetText();   // got
        }
        AccessModifier accessCopy = access;
        std::vector<Stmt *> block;
        while (!expect(TkRBrace)) {
            block.push_back(ParseStmt());
        }
        return createNode<FunDeclStmt>(name, retType, args, block, false, accessCopy, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseRetStmt() {
        Token firstTok = consume();
        Expr *expr = nullptr;
        if (!_curTok.Is(TkSemi)) {
            expr = parseExpr(PrecLowest);
        }
        return createNode<RetStmt>(expr, access, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseIfElseStmt() {
        AccessModifier accessCopy = access;
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
        return createNode<IfElseStmt>(cond, thenBranch, elseBranch, accessCopy, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseForLoopStmt() {
        AccessModifier accessCopy = access;
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
        return createNode<ForLoopStmt>(indexator, cond, iteration, block, accessCopy, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseStructStmt() {
        AccessModifier accessCopy = access;
        Token firstTok = consume();
        std::string name = _curTok.GetText();
        if (!expect(TkId)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedId)
                << getRangeFromTok(_curTok)
                << _curTok.GetText();
        }
        if (!expect(TkLBrace)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                << getRangeFromTok(_curTok)
                << "{"                  // expected
                << _curTok.GetText();   // got
        }
        types.emplace(name, ASTType(ASTTypeKind::Struct, name, false, 0));
        std::vector<Stmt *> body;
        while (!expect(TkRBrace)) {
            body.push_back(ParseStmt());
        }
        return createNode<StructStmt>(name, body, accessCopy, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseImplStmt() {
        AccessModifier accessCopy = access;
        Token firstTok = consume();
        std::string traitName = _curTok.GetText();
        std::string structName = "";
        if (!expect(TkId)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedId)
                << getRangeFromTok(_curTok)
                << _curTok.GetText();
        }
        if (expect(TkFor)) {
            structName = _curTok.GetText();
            if (!expect(TkId)) {
                _diag.Report(_curTok.GetLoc(), ErrExpectedId)
                    << getRangeFromTok(_curTok)
                    << _curTok.GetText();
            }
        }
        else {
            structName = traitName;
            traitName = "";
        }
        if (!expect(TkLBrace)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                << getRangeFromTok(_curTok)
                << "{"                  // expected
                << _curTok.GetText();   // got
        }
        std::vector<Stmt *> body;
        while (!expect(TkRBrace)) {
            body.push_back(ParseStmt());
        }
        return createNode<ImplStmt>(traitName, structName, body, accessCopy, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseTraitDeclStmt() {
        AccessModifier accessCopy = access;
        Token firstTok = consume();
        std::string name = _curTok.GetText();
        if (!expect(TkId)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedId)
                << getRangeFromTok(_curTok)
                << _curTok.GetText();
        }
        if (!expect(TkLBrace)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                << getRangeFromTok(_curTok)
                << "{"                  // expected
                << _curTok.GetText();   // got
        }
        std::vector<Stmt *> body;
        while (!expect(TkRBrace)) {
            body.push_back(ParseStmt());
        }
        types.emplace(name, ASTType(ASTTypeKind::Trait, name, false, 0));
        return createNode<TraitDeclStmt>(name, body, accessCopy, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseDelStmt() {
        AccessModifier accessCopy = access;
        Token firstTok = consume();
        Expr *expr = parseExpr(PrecLowest);
        return createNode<DelStmt>(expr, accessCopy, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseImportStmt() {
        AccessModifier accessCopy = access;
        Token firstTok = consume();
        std::string path;
        bool isLocalImport = false;
        if (_curTok.GetKind() == TkStrLit) {
            path = consume().GetText();
            isLocalImport = true;
        }
        else {
            while (_curTok.GetKind() != TkSemi) {
                path += _curTok.GetText() == "." ? "/" : _curTok.GetText();
                consume();
            }
        }
        if (isLocalImport) {
            unsigned bufferID = _srcMgr.FindBufferContainingLoc(firstTok.GetLoc());
            if (bufferID == 0) {
                _diag.Report(firstTok.GetLoc(), ErrCannotFindModule)
                    << llvm::SMRange(firstTok.GetLoc(), firstTok.GetLoc())
                    << path;
                return nullptr;
            }
            const llvm::MemoryBuffer *buffer = _srcMgr.getMemoryBuffer(bufferID);
            if (!buffer) {
                _diag.Report(firstTok.GetLoc(), ErrCannotFindModule)
                    << llvm::SMRange(firstTok.GetLoc(), firstTok.GetLoc())
                    << path;
                return nullptr;
            }
            path = llvm::sys::path::parent_path(buffer->getBufferIdentifier()).str() + path;
        }
        else {
            path = libsPath + path;
        }
        auto bufferOrErr = llvm::MemoryBuffer::getFile(path + ".mr");
        if (std::error_code ec = bufferOrErr.getError()) {
            path += "/mod";
        }
        _modManager.LoadModule(path + ".mr", AccessPub, _srcMgr);
        return createNode<ImportStmt>(path, isLocalImport, accessCopy, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Stmt *
    Parser::parseModuleDeclStmt() {
        AccessModifier accessCopy = access;
        Token firstTok = consume();
        std::string name = _curTok.GetText();
        if (!expect(TkId)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedId)
                << getRangeFromTok(_curTok)
                << _curTok.GetText();
        }
        if (!expect(TkLBrace)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                << getRangeFromTok(_curTok)
                << "{"                  // expected
                << _curTok.GetText();   // got
        }
        std::vector<Stmt *> body;
        while (!expect(TkRBrace)) {
            body.push_back(ParseStmt());
        }
        return createNode<ModuleDeclStmt>(name, body, accessCopy, firstTok.GetLoc(), _curTok.GetLoc());
    }

    Argument
    Parser::parseArgument() {
        std::string name = _curTok.GetText();
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
        std::string text = _curTok.GetText();
        switch (_curTok.GetKind()) {
            case TkId: {
                Token nameToken = consume();
                switch (_curTok.GetKind()) {
                    case TkLParen: {
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
                        Expr *expr = createNode<FunCallExpr>(nameToken.GetText(), args, nameToken.GetLoc(), _curTok.GetLoc());
                        if (expect(TkDot)) {
                            return parseChainExpr(expr);
                        }
                        return expr;
                    }
                    case TkLBrace: {
                        consume();
                        std::vector<std::pair<std::string, Expr *>> initializer;
                        while (!expect(TkRBrace)) {
                            std::string name = _curTok.GetText();
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
                            initializer.push_back({ name, parseExpr(PrecLowest) });
                            if (!_curTok.Is(TkRBrace)) {
                                if (!expect(TkComma)) {
                                    _diag.Report(_curTok.GetLoc(), ErrExpectedToken)
                                        << getRangeFromTok(_curTok)
                                        << ","                  // expected
                                        << _curTok.GetText();   // got
                                }
                            }
                        }
                        return createNode<StructExpr>(nameToken.GetText(), initializer, nameToken.GetLoc(), _curTok.GetLoc());
                    }
                    default: {
                        Expr *expr = createNode<VarExpr>(nameToken.GetText(), nameToken.GetLoc(), _curTok.GetLoc());
                        if (expect(TkDot)) {
                            return parseChainExpr(expr);
                        }
                        return expr;
                    }
                }
            }
            #define LIT(kind, type_val, field, val) \
                createNode<LiteralExpr>(ASTVal(ASTType(ASTTypeKind::kind, type_val, true, 0), \
                                               ASTValData { .field = (val) }, false, false), consume().GetLoc(), _curTok.GetLoc())
            case TkBoolLit:
                return LIT(Bool, "bool", boolVal, text == "true");
            case TkCharLit:
                return LIT(Char, "char", charVal, text[0]);
            case TkI16Lit:
                return LIT(I16, "i16", i16Val, static_cast<short>(std::stoi(text)));
            case TkI32Lit:
                return LIT(I32, "i32", i32Val, std::stoi(text));
            case TkI64Lit:
                return LIT(I64, "i64", i64Val, std::stol(text));
            case TkF32Lit:
                return LIT(F32, "f32", f32Val, std::stof(text));
            case TkF64Lit:
                return LIT(F64, "f64", f64Val, std::stod(text));
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
            case TkNil: {
                return createNode<NilExpr>(consume().GetLoc(), _curTok.GetLoc());
            }
            case TkStar: {
                Token star = consume();
                Expr *expr = parsePrefixExpr();
                return createNode<DerefExpr>(expr, star.GetLoc(), _curTok.GetLoc());
            }
            case TkAnd: {
                Token amp = consume();
                Expr *expr = parsePrefixExpr();
                return createNode<RefExpr>(expr, amp.GetLoc(), _curTok.GetLoc());
            }
            case TkNew: {
                Token newTok = consume();
                ASTType type;
                StructExpr *se = nullptr;
                if (_curTok.GetKind() == TkId && _nextTok.GetKind() == TkLBrace) {
                    se = llvm::cast<StructExpr>(parsePrefixExpr());
                    type = ASTType(ASTTypeKind::Struct, se->GetName(), true, 0);
                }
                else {
                    type = consumeType();
                }
                return createNode<NewExpr>(type, se, newTok.GetLoc(), _curTok.GetLoc());
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

    Expr *
    Parser::parseChainExpr(Expr *base) {
        Token nameToken = _curTok; 
        if (!expect(TkId)) {
            _diag.Report(_curTok.GetLoc(), ErrExpectedId)
                << getRangeFromTok(_curTok)
                << _curTok.GetText();
        }
        Expr *expr;
        if (expect(TkLParen)) {
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
            expr = createNode<MethodCallExpr>(base, nameToken.GetText(), args, nameToken.GetLoc(), _curTok.GetLoc());
        }
        else {
            expr = createNode<FieldAccessExpr>(base, nameToken.GetText(), nameToken.GetLoc(), _curTok.GetLoc());
        }
        if (expect(TkDot)) {
            return parseChainExpr(expr);
        }
        return expr;
    }
    
    Token
    Parser::consume() {
        _lastTok = _curTok;
        _curTok = _nextTok;
        _nextTok = _lex.NextToken();
        return _lastTok;
    }

    ASTType
    Parser::consumeType() {
        bool isConst = expect(TkConst);
        unsigned char pointerDepth = 0;
        for (; expect(TkStar); ++pointerDepth);
        Token type = consume();
        switch (type.GetKind()) {
            #define TYPE(kind, type_val) ASTType(ASTTypeKind::kind, type_val, isConst, pointerDepth)
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
            case TkNoth:
                return TYPE(Noth, "noth");
            case TkId: {
                           std::cout << types.size() << '\n';
                if (types.find(type.GetText()) == types.end()) {
                    _diag.Report(_lastTok.GetLoc(), ErrUndeclaredType)
                        << llvm::SMRange(_lastTok.GetLoc(), _curTok.GetLoc())
                        << type.GetText();
                    return TYPE(I32, "i32");
                }
                ASTTypeKind t = types.at(type.GetText()).GetTypeKind();
                if (t == ASTTypeKind::Struct) {
                    return TYPE(Struct, type.GetText());
                }
                return TYPE(Trait, type.GetText());
            }
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
