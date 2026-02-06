#include <marble/AST/Printer.h>
#include <llvm/Support/raw_ostream.h>

namespace marble {
    void
    ASTPrinter::VisitVarDeclStmt(VarDeclStmt *vds) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(VarDeclStmt: " << vds->GetType().ToString() << ' ' << vds->GetName() << ' ';
        if (vds->GetExpr()) {
            int spaces = _spaces;
            _spaces = 0;
            Visit(vds->GetExpr());
            _spaces = spaces;
        }
        llvm::outs() << ')';
    }

    void
    ASTPrinter::VisitVarAsgnStmt(VarAsgnStmt *vas) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(VarAsgnStmt: " << vas->GetName() << " = ";
        int spaces = _spaces;
        _spaces = 0;
        Visit(vas->GetExpr());
        _spaces = spaces;
        llvm::outs() << ')';
    }

    void
    ASTPrinter::VisitFunDeclStmt(FunDeclStmt *fds) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(FunDeclStmt: " << fds->GetRetType().ToString() << ' ' << fds->GetName() << " (";
        for (int i = 0; i < fds->GetArgs().size(); ++i) {
            llvm::outs() << fds->GetArgs()[i].GetType().ToString() << ' ' << fds->GetArgs()[i].GetName();
            if (i < fds->GetArgs().size() - 1) {
                llvm::outs() << ", ";
            }
        }
        llvm::outs() << ")";
        if (!fds->IsDeclaration()) {
            llvm::outs() << " {";
            if (fds->GetBody().size() != 0) {
                llvm::outs() << '\n';
            }
            _spaces += 2;
            for (auto stmt : fds->GetBody()) {
                Visit(stmt);
                llvm::outs() << '\n';
            }
            _spaces -= 2;
            if (fds->GetBody().size() != 0) {
                llvm::outs() << std::string(_spaces, ' ');
            }
            llvm::outs() << "}";
        }
        llvm::outs() << ")";
    }

    void
    ASTPrinter::VisitFunCallStmt(FunCallStmt *fcs) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(FunCallStmt: " << fcs->GetName() << " (";
        for (int i = 0; i < fcs->GetArgs().size(); ++i) {
            Visit(fcs->GetArgs()[i]);
            if (i < fcs->GetArgs().size() - 1) {
                llvm::outs() << ", ";
            }
        }
        llvm::outs() << ')';
    }

    void
    ASTPrinter::VisitRetStmt(RetStmt *rs) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(RetStmt: ";
        if (rs->GetExpr()) {
            int spaces = _spaces;
            _spaces = 0;
            Visit(rs->GetExpr());
            _spaces = spaces;
        }
        llvm::outs() << ')';
    }

    void
    ASTPrinter::VisitIfElseStmt(IfElseStmt *ies) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(IfElseStmt: ";
        int spaces = _spaces;
        _spaces = 0;
        Visit(ies->GetCondition());
        _spaces = spaces;

        llvm::outs() << "then {";
        if (ies->GetThenBody().size() != 0) {
            llvm::outs() << '\n';
        }
        _spaces += 2;
        for (auto stmt : ies->GetThenBody()) {
            Visit(stmt);
            llvm::outs() << '\n';
        }
        _spaces -= 2;

        if (ies->GetThenBody().size() != 0) {
            llvm::outs() << std::string(_spaces, ' ');
        }
        llvm::outs() << "} else {";
        for (auto stmt : ies->GetElseBody()) {
            Visit(stmt);
            llvm::outs() << '\n';
        }
        _spaces += 2;
        for (auto stmt : ies->GetElseBody()) {
            Visit(stmt);
            llvm::outs() << '\n';
        }
        _spaces -= 2;

        if (ies->GetElseBody().size() != 0) {
            llvm::outs() << std::string(_spaces, ' ');
        }
        llvm::outs() << "})";
    }
    
    void
    ASTPrinter::VisitForLoopStmt(ForLoopStmt *fls) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(FopLoopStmt: ";
        int spaces = _spaces;
        _spaces = 0;
        if (fls->GetIndexator()) {
            Visit(fls->GetIndexator());
            llvm::outs() << ", ";
        }
        Visit(fls->GetCondition());
        if (fls->GetIteration()) {
            llvm::outs() << ", ";
            Visit(fls->GetIteration());
        }
        _spaces = spaces;

        llvm::outs() << " {";
        if (fls->GetBody().size() != 0) {
            llvm::outs() << std::string(_spaces, ' ');
        }
        _spaces += 2;
        for (auto stmt : fls->GetBody()) {
            Visit(stmt);
            llvm::outs() << '\n';
        }
        _spaces -= 2;
        if (fls->GetBody().size() != 0) {
            llvm::outs() << std::string(_spaces, ' ');
        }
        llvm::outs() << "})";
    }

    void
    ASTPrinter::VisitBreakStmt(BreakStmt *bs) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(BreakStmt)";
    }

    void
    ASTPrinter::VisitContinueStmt(ContinueStmt *cs) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(ContinueStmt)";
    }

    void
    ASTPrinter::VisitStructStmt(StructStmt *ss) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(StructStmt: " << ss->GetName() << " (";
        llvm::outs() << " {";
        if (ss->GetBody().size() != 0) {
            llvm::outs() << '\n';
        }
        _spaces += 2;
        for (auto stmt : ss->GetBody()) {
            Visit(stmt);
            llvm::outs() << '\n';
        }
        _spaces -= 2;
        if (ss->GetBody().size() != 0) {
            llvm::outs() << std::string(_spaces, ' ');
        }
        llvm::outs() << "})";
    }

    void
    ASTPrinter::VisitFieldAsgnStmt(FieldAsgnStmt *fas) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(FieldAsgnStmt: " << fas->GetName() << " from ";
        int spaces = _spaces;
        _spaces = 0;
        Visit(fas->GetObject());
        llvm::outs() << " = ";
        Visit(fas->GetExpr());
        _spaces = spaces;
        llvm::outs() << ')';
    }

    void
    ASTPrinter::VisitImplStmt(ImplStmt *is) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(ImplStmt: ";
        if (is->GetTraitName() != "") {
            llvm::outs() << is->GetTraitName() << " for " << is->GetStructName();
        }
        else {
            llvm::outs() << is->GetStructName();
        }
        llvm::outs() << " {";
        if (is->GetBody().size() != 0) {
            llvm::outs() << '\n';
        }
        _spaces += 2;
        for (auto stmt : is->GetBody()) {
            Visit(stmt);
            llvm::outs() << '\n';
        }
        _spaces -= 2;
        if (is->GetBody().size() != 0) {
            llvm::outs() << std::string(_spaces, ' ');
        }
        llvm::outs() << "})";
    }
    
    void
    ASTPrinter::VisitMethodCallStmt(MethodCallStmt *mcs) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(MethodCallStmt: " << mcs->GetName() << " (";
        for (int i = 0; i < mcs->GetArgs().size(); ++i) {
            Visit(mcs->GetArgs()[i]);
            if (i < mcs->GetArgs().size() - 1) {
                llvm::outs() << ", ";
            }
        }
        llvm::outs() << ") from ";
        int spaces = _spaces;
        _spaces = 0;
        Visit(mcs->GetObject());
        _spaces = spaces;
    }

    void
    ASTPrinter::VisitTraitDeclStmt(TraitDeclStmt *tds) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(TraitDeclStmt: " << tds->GetName() << " {";
        if (tds->GetBody().size() != 0) {
            llvm::outs() << '\n';
        }
        _spaces += 2;
        for (auto stmt : tds->GetBody()) {
            Visit(stmt);
            llvm::outs() << '\n';
        }
        _spaces -= 2;
        if (tds->GetBody().size() != 0) {
            llvm::outs() << std::string(_spaces, ' ');
        }
        llvm::outs() << "})";
    }

    void
    ASTPrinter::VisitEchoStmt(EchoStmt *es) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(EchoStmt: ";
        int spaces = _spaces;
        _spaces = 0;
        Visit(es->GetRHS());
        _spaces = spaces;
        llvm::outs() << ')';
    }
    
    void
    ASTPrinter::VisitBinaryExpr(BinaryExpr *be) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(BinaryExpr: ";
        int spaces = _spaces;
        _spaces = 0;
        Visit(be->GetLHS());
        llvm::outs() << ' ' << be->GetOp().GetText() << ' ';
        Visit(be->GetRHS());
        _spaces = spaces;
        llvm::outs() << ')';
    }

    void
    ASTPrinter::VisitUnaryExpr(UnaryExpr *ue) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(UnaryExpr: " << ue->GetOp().GetText() << ' ';
        int spaces = _spaces;
        _spaces = 0;
        Visit(ue->GetRHS());
        _spaces = spaces;
        llvm::outs() << ')';
    }

    void
    ASTPrinter::VisitVarExpr(VarExpr *ve) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(VarExpr: " << ve->GetName() << ')';
    }

    void
    ASTPrinter::VisitLiteralExpr(LiteralExpr *le) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(LiteralExpr: " << le->GetVal().GetType().ToString() << ' ' << le->GetVal().ToString() << ')';
    }

    void
    ASTPrinter::VisitFunCallExpr(FunCallExpr *fce) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(FunCallExpr: " << fce->GetName() << " (";
        for (int i = 0; i < fce->GetArgs().size(); ++i) {
            Visit(fce->GetArgs()[i]);
            if (i < fce->GetArgs().size() - 1) {
                llvm::outs() << ", ";
            }
        }
        llvm::outs() << "))";
    }

    void
    ASTPrinter::VisitStructExpr(StructExpr *se) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(StructExpr: " << se->GetName() << " { ";
        for (int i = 0; i < se->GetInitializer().size(); ++i) {
            llvm::outs() << se->GetInitializer()[i].first << ": ";
            Visit(se->GetInitializer()[i].second);
            if (i < se->GetInitializer().size() - 1) {
                llvm::outs() << ", ";
            }
        }
        llvm::outs() << " })";
    }

    void
    ASTPrinter::VisitFieldAccessExpr(FieldAccessExpr *fae) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(FieldAccessExpr: " << fae->GetName() << " from ";
        int spaces = _spaces;
        _spaces = 0;
        Visit(fae->GetObject());
        _spaces = spaces;
        llvm::outs() << ')';
    }

    void
    ASTPrinter::VisitMethodCallExpr(MethodCallExpr *mce) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(MethodCallExpr: " << mce->GetName() << " (";
        for (int i = 0; i < mce->GetArgs().size(); ++i) {
            Visit(mce->GetArgs()[i]);
            if (i < mce->GetArgs().size() - 1) {
                llvm::outs() << ", ";
            }
        }
        llvm::outs() << ") from ";
        int spaces = _spaces;
        _spaces = 0;
        Visit(mce->GetObject());
        _spaces = spaces;
        llvm::outs() << ')';
    }
}
