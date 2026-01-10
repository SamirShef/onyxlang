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
}