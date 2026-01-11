#include <onyx/AST/Printer.h>
#include <llvm/Support/raw_ostream.h>

namespace onyx {
    void
    ASTPrinter::visitVarDeclStmt(VarDeclStmt *vds) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(VarDeclStmt: " << vds->GetType().ToString() << ' ' << vds->GetName().str() << ' ';
        if (vds->GetExpr()) {
            int spaces = _spaces;
            _spaces = 0;
            visit(vds->GetExpr());
            _spaces = spaces;
        }
        llvm::outs() << ')';
    }

    void
    ASTPrinter::visitFunDeclStmt(FunDeclStmt *fds) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(FunDeclStmt: " << fds->GetRetType().ToString() << ' ' << fds->GetName().str() << " (";
        for (int i = 0; i < fds->GetArgs().size(); ++i) {
            llvm::outs() << fds->GetArgs()[i].GetType().ToString() << ' ' << fds->GetArgs()[i].GetName().str();
            if (i < fds->GetArgs().size() - 1) {
                llvm::outs() << ", ";
            }
        }
        llvm::outs() << " {";
        _spaces += 2;
        for (auto stmt : fds->GetBody()) {
            visit(stmt);
        }
        _spaces -= 2;
        llvm::outs() << "})";
    }

    void
    ASTPrinter::visitRetStmt(RetStmt *rs) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(RetStmt: ";
        if (rs->GetExpr()) {
            int spaces = _spaces;
            _spaces = 0;
            visit(rs->GetExpr());
            _spaces = spaces;
        }
        llvm::outs() << ')';
    }

    void
    ASTPrinter::visitBinaryExpr(BinaryExpr *be) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(BinaryExpr: ";
        int spaces = _spaces;
        _spaces = 0;
        visit(be->GetLHS());
        llvm::outs() << ' ' << be->GetOp().GetText().str() << ' ';
        visit(be->GetLHS());
        _spaces = spaces;
        llvm::outs() << ')';
    }

    void
    ASTPrinter::visitUnaryExpr(UnaryExpr *ue) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(UnaryExpr: " << ue->GetOp().GetText().str() << ' ';
        int spaces = _spaces;
        _spaces = 0;
        visit(ue->GetRHS());
        _spaces = spaces;
        llvm::outs() << ')';
    }

    void
    ASTPrinter::visitVarExpr(VarExpr *ve) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(VarExpr: " << ve->GetName().str() << ')';
    }

    void
    ASTPrinter::visitLiteralExpr(LiteralExpr *le) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(LiteralExpr: " << le->GetVal().GetType().ToString() << ' ' << le->GetVal().ToString() << ')';
    }

    void
    ASTPrinter::visitFunCallExpr(FunCallExpr *fce) {
        llvm::outs() << std::string(_spaces, ' ');
        llvm::outs() << "(FunCallExpr: " << fce->GetName().str() << " (";
        for (int i = 0; i < fce->GetArgs().size(); ++i) {
            visit(fce->GetArgs()[i]);
            if (i < fce->GetArgs().size() - 1) {
                llvm::outs() << ", ";
            }
        }
        llvm::outs() << ')';
    }
}