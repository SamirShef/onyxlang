#include <onyx/Sema/Semantic.h>
#include <cmath>

namespace onyx {
    static std::unordered_map<ASTTypeKind, std::vector<ASTTypeKind>> implicitlyCastAllowed {
        { ASTTypeKind::Char, { ASTTypeKind::I16, ASTTypeKind::I32, ASTTypeKind::I64, ASTTypeKind::F32, ASTTypeKind::F64 } },
        { ASTTypeKind::I16,  { ASTTypeKind::I32, ASTTypeKind::I64, ASTTypeKind::F32, ASTTypeKind::F64                       } },
        { ASTTypeKind::I32,  { ASTTypeKind::I64, ASTTypeKind::F32, ASTTypeKind::F64                                             } },
        { ASTTypeKind::I64,  { ASTTypeKind::F32, ASTTypeKind::F64                                                                   } },
        { ASTTypeKind::F32,  { ASTTypeKind::F64                                                                                         } },
    };
    
    std::optional<ASTVal>
    SemanticAnalyzer::visitVarDeclStmt(VarDeclStmt *vds) {
        if (variableExists(vds->GetName().str())) {
            _diag.Report(llvm::SMLoc::getFromPointer(vds->GetName().data()), ErrRedefinitionVar)
                << getRange(llvm::SMLoc::getFromPointer(vds->GetName().data()), vds->GetName().size())
                << vds->GetName();
        }
        else {
            std::optional<ASTVal> val = vds->GetExpr() != nullptr ? visit(vds->GetExpr()) : ASTVal::GetDefaultByType(vds->GetType());
            Variable var { .Name = vds->GetName(), .Type = vds->GetType(), .Val = val, .IsConst = vds->IsConst() };
            if (vds->GetExpr()) {
                implicitlyCast(var.Val.value(), var.Type, vds->GetExpr()->GetStartLoc(), vds->GetExpr()->GetEndLoc());
            }
            _vars.top().emplace(vds->GetName(), var);
        }
        return std::nullopt;
    }
    
    std::optional<ASTVal>
    SemanticAnalyzer::visitBinaryExpr(BinaryExpr *be) {
        checkBinaryExpr(be);
        std::optional<ASTVal> lhs = visit(be->GetLHS());
        if (lhs == std::nullopt) {
            return lhs;
        }
        std::optional<ASTVal> rhs = visit(be->GetRHS());
        if (rhs == std::nullopt) {
            return rhs;
        }
        double lhsVal = lhs->AsDouble();
        double rhsVal = rhs->AsDouble();
        double res;
        switch (be->GetOp().GetKind()) {
            #define EVAL(op) lhsVal op rhsVal
            case TkPlus:
                res = EVAL(+);
                break;
            case TkMinus:
                res = EVAL(+);
                break;
            case TkStar:
                res = EVAL(*);
                break;
            case TkSlash:
                res = EVAL(/);
                break;
            case TkPercent:
                res = std::fmod(lhsVal, rhsVal);
                break;
            case TkLogAnd:
                res = EVAL(&&);
                break;
            case TkLogOr:
                res = EVAL(||);
                break;
            case TkAnd:
                res = static_cast<long>(lhsVal) & static_cast<long>(rhsVal);
                break;
            case TkOr:
                res = static_cast<long>(lhsVal) | static_cast<long>(rhsVal);
                break;
            case TkGt:
                res = EVAL(>);
                break;
            case TkGtEq:
                res = EVAL(>=);
                break;
            case TkLt:
                res = EVAL(<);
                break;
            case TkLtEq:
                res = EVAL(<=);
                break;
            case TkEqEq:
                res = EVAL(==);
                break;
            case TkNotEq:
                res = EVAL(!=);
                break;
            default: {}
            #undef EVAL
        }
        return ASTVal::GetVal(res, ASTType::GetCommon(lhs->GetType(), rhs->GetType()));
    }
    
    std::optional<ASTVal>
    SemanticAnalyzer::visitUnaryExpr(UnaryExpr *ue) {
        std::optional<ASTVal> rhs = visit(ue->GetRHS());
        if (rhs == std::nullopt) {
            return rhs;
        }
        double val = rhs->AsDouble();
        switch (ue->GetOp().GetKind()) {
            case TkMinus:
                if (rhs->GetType().GetTypeKind() < ASTTypeKind::Char || rhs->GetType().GetTypeKind() > ASTTypeKind::F64) {
                    _diag.Report(ue->GetRHS()->GetStartLoc(), ErrTypeMismatchNotNum)
                        << llvm::SMRange(ue->GetRHS()->GetStartLoc(), ue->GetRHS()->GetEndLoc())
                        << rhs->GetType().ToString();
                }
                val *= -1;
                break;
            case TkBang:
                if (rhs->GetType().GetTypeKind() != ASTTypeKind::Bool) {
                    _diag.Report(ue->GetRHS()->GetStartLoc(), ErrTypeMismatchNotBool)
                        << llvm::SMRange(ue->GetRHS()->GetStartLoc(), ue->GetRHS()->GetEndLoc())
                        << rhs->GetType().ToString();
                }
                val = !val;
                break;
            default: {}
        }
        return ASTVal::GetVal(val, rhs->GetType());
    }
    
    std::optional<ASTVal>
    SemanticAnalyzer::visitVarExpr(VarExpr *ve) {
        auto varsCopy = _vars;
        while (!varsCopy.empty()) {
            if (auto var = varsCopy.top().find(ve->GetName().str()); var != varsCopy.top().end()) {
                return var->second.Val;
            }
            varsCopy.pop();
        }
        _diag.Report(ve->GetStartLoc(), ErrUndeclaredVariable)
            << getRange(ve->GetStartLoc(), ve->GetName().size())
            << ve->GetName();
        return std::nullopt;
    }
    
    std::optional<ASTVal>
    SemanticAnalyzer::visitLiteralExpr(LiteralExpr *le) {
        return le->GetVal();
    }

    llvm::SMRange
    SemanticAnalyzer::getRange(llvm::SMLoc start, int len) const {
        return llvm::SMRange(start, llvm::SMLoc::getFromPointer(start.getPointer() + len));
    }

    void
    SemanticAnalyzer::checkBinaryExpr(BinaryExpr *be) {
        ASTVal lhs = visit(be->GetRHS()).value();
        ASTVal rhs = visit(be->GetRHS()).value();
        switch (be->GetOp().GetKind()) {
            case TkPlus:
            case TkMinus:
            case TkStar:
            case TkSlash:
            case TkPercent:
                if (!(lhs.GetType().GetTypeKind() >= ASTTypeKind::Char && lhs.GetType().GetTypeKind() <= ASTTypeKind::F64 &&
                    rhs.GetType().GetTypeKind() >= ASTTypeKind::Char && rhs.GetType().GetTypeKind() <= ASTTypeKind::F64)) {
                    _diag.Report(be->GetStartLoc(), ErrUnsupportedTypeForOperator)
                        << llvm::SMRange(be->GetStartLoc(), be->GetEndLoc())
                        << be->GetOp().GetText().str()
                        << lhs.GetType().ToString()
                        << rhs.GetType().ToString();
                }
                break;
            case TkLogAnd:
            case TkLogOr:
                if (lhs.GetType().GetTypeKind() != ASTTypeKind::Bool ||
                    rhs.GetType().GetTypeKind() != ASTTypeKind::Bool) {
                    _diag.Report(be->GetStartLoc(), ErrUnsupportedTypeForOperator)
                        << llvm::SMRange(be->GetStartLoc(), be->GetEndLoc())
                        << be->GetOp().GetText().str()
                        << lhs.GetType().ToString()
                        << rhs.GetType().ToString();
                }
                break;
            case TkAnd:
            case TkOr:
                if (!(lhs.GetType().GetTypeKind() >= ASTTypeKind::Char && lhs.GetType().GetTypeKind() <= ASTTypeKind::I64 &&
                    rhs.GetType().GetTypeKind() >= ASTTypeKind::Char && rhs.GetType().GetTypeKind() <= ASTTypeKind::I64)) {
                    _diag.Report(be->GetStartLoc(), ErrUnsupportedTypeForOperator)
                        << llvm::SMRange(be->GetStartLoc(), be->GetEndLoc())
                        << be->GetOp().GetText().str()
                        << lhs.GetType().ToString()
                        << rhs.GetType().ToString();
                }
                break;
            case TkGt:
            case TkGtEq:
            case TkLt:
            case TkLtEq:
            case TkEqEq:
            case TkNotEq:
                if (!ASTType::HasCommon(lhs.GetType(), rhs.GetType())) {
                    _diag.Report(be->GetStartLoc(), ErrUnsupportedTypeForOperator)
                        << llvm::SMRange(be->GetStartLoc(), be->GetEndLoc())
                        << be->GetOp().GetText().str()
                        << lhs.GetType().ToString()
                        << rhs.GetType().ToString();
                }
                break;
            default: {}
        }
    }

    bool
    SemanticAnalyzer::variableExists(std::string name) const {
        auto varsCopy = _vars;
        while (!varsCopy.empty()) {
            if (auto var = varsCopy.top().find(name); var != varsCopy.top().end()) {
                return true;
            }
            varsCopy.pop();
        }
        return false;
    }

    bool
    SemanticAnalyzer::canImplicitlyCast(ASTVal src, ASTType expectType) const {
        if (src.GetType() == expectType) {
            return true;
        }
        if (auto it = implicitlyCastAllowed.find(src.GetType().GetTypeKind()); it != implicitlyCastAllowed.end()) {
            return std::find(it->second.begin(), it->second.end(), expectType.GetTypeKind()) != it->second.end();
        }
        return false;
    }
    
    ASTVal
    SemanticAnalyzer::implicitlyCast(ASTVal src, ASTType expectType, llvm::SMLoc startLoc, llvm::SMLoc endLoc) const {
        if (src.GetType() == expectType) {
            return src;
        }
        if (auto it = implicitlyCastAllowed.find(src.GetType().GetTypeKind()); it != implicitlyCastAllowed.end()) {
            if (std::find(it->second.begin(), it->second.end(), expectType.GetTypeKind()) != it->second.end()) {
                return src.Cast(expectType);
            }
        }
        _diag.Report(startLoc, ErrCannotCast)
            << llvm::SMRange(startLoc, endLoc)
            << src.GetType().ToString()
            << expectType.ToString();
        return ASTVal::GetDefaultByType(ASTType::GetNothType());
    }
}