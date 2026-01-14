#include <onyx/AST/Printer.h>
#include <llvm/Support/raw_ostream.h>

namespace onyx {
    void
    ASTPrinter::VisitVarDeclStmt(VarDeclStmt *vds) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(VarDeclStmt: " << vds->GetType().ToString() << ' ' << vds->GetName().str() << ' ';
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
        llvm::outs() << "(VarAsgnStmt: " << vas->GetName().str() << " = ";
        int spaces = _spaces;
        _spaces = 0;
        Visit(vas->GetExpr());
        _spaces = spaces;
        llvm::outs() << ')';
    }

    void
    ASTPrinter::VisitFunDeclStmt(FunDeclStmt *fds) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(FunDeclStmt: " << fds->GetRetType().ToString() << ' ' << fds->GetName().str() << " (";
        for (int i = 0; i < fds->GetArgs().size(); ++i) {
            llvm::outs() << fds->GetArgs()[i].GetType().ToString() << ' ' << fds->GetArgs()[i].GetName().str();
            if (i < fds->GetArgs().size() - 1) {
                llvm::outs() << ", ";
            }
        }
        llvm::outs() << " {\n";
        _spaces += 2;
        for (auto stmt : fds->GetBody()) {
            Visit(stmt);
            llvm::outs() << '\n';
        }
        _spaces -= 2;
        llvm::outs() << "})";
    }

    void
    ASTPrinter::VisitFunCallStmt(FunCallStmt *fcs) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(FunCallStmt: " << fcs->GetName().str() << " (";
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
        _spaces += 2;
        for (auto stmt : ies->GetThenBody()) {
            Visit(stmt);
            llvm::outs() << '\n';
        }
        _spaces -= 2;

        llvm::outs() << "else {";
        _spaces += 2;
        for (auto stmt : ies->GetElseBody()) {
            Visit(stmt);
            llvm::outs() << '\n';
        }
        _spaces -= 2;

        llvm::outs() << "})";
    }
    
    void
    ASTPrinter::VisitForLoopStmt(ForLoopStmt *fls) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(IfElseStmt: ";
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
        _spaces += 2;
        for (auto stmt : fls->GetBody()) {
            Visit(stmt);
            llvm::outs() << '\n';
        }
        _spaces -= 2;
        llvm::outs() << "})";
    }

    void
    ASTPrinter::VisitBinaryExpr(BinaryExpr *be) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(BinaryExpr: ";
        int spaces = _spaces;
        _spaces = 0;
        Visit(be->GetLHS());
        llvm::outs() << ' ' << be->GetOp().GetText().str() << ' ';
        Visit(be->GetLHS());
        _spaces = spaces;
        llvm::outs() << ')';
    }

    void
    ASTPrinter::VisitUnaryExpr(UnaryExpr *ue) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(UnaryExpr: " << ue->GetOp().GetText().str() << ' ';
        int spaces = _spaces;
        _spaces = 0;
        Visit(ue->GetRHS());
        _spaces = spaces;
        llvm::outs() << ')';
    }

    void
    ASTPrinter::VisitVarExpr(VarExpr *ve) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(VarExpr: " << ve->GetName().str() << ')';
    }

    void
    ASTPrinter::VisitLiteralExpr(LiteralExpr *le) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(LiteralExpr: " << le->GetVal().GetType().ToString() << ' ' << le->GetVal().ToString() << ')';
    }

    void
    ASTPrinter::VisitFunCallExpr(FunCallExpr *fce) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(FunCallExpr: " << fce->GetName().str() << " (";
        for (int i = 0; i < fce->GetArgs().size(); ++i) {
            Visit(fce->GetArgs()[i]);
            if (i < fce->GetArgs().size() - 1) {
                llvm::outs() << ", ";
            }
        }
        llvm::outs() << ')';
    }
}