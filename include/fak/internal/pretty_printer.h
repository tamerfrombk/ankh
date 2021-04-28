#pragma once

#include <initializer_list>
#include <string>

#include <fak/lang/expr.h>
#include <fak/lang/token.h>
#include <fak/log.h>

namespace fk::internal {

class pretty_printer
    : public fk::lang::expression_visitor<fk::lang::expr_result>
{
public:
    inline virtual fk::lang::expr_result visit(fk::lang::binary_expression *expr) override
    {
        return paren(expr->op.str, { expr->left.get(), expr->right.get() });
    }

    inline virtual fk::lang::expr_result visit(fk::lang::unary_expression *expr) override
    {
        return paren(expr->op.str, { expr->right.get() });
    }

    inline virtual fk::lang::expr_result visit(fk::lang::literal_expression *expr) override
    {
        return paren(expr->literal.str, {});
    }

    inline virtual fk::lang::expr_result visit(fk::lang::paren_expression *expr) override
    {
        return paren("paren", { expr->expr.get() });
    }

private:
    inline fk::lang::expr_result paren(const std::string& op, std::initializer_list<fk::lang::expression*> exprs)
    {
        std::string result;

        result += "( ";
        result += op;
        for (const auto expr : exprs) {
            result += " ";
            result += expr->accept(this).str;
        }
        result += " )";

        fk::lang::expr_result expr;
        expr.type = fk::lang::expr_result_type::RT_STRING;
        expr.str  = result;

        return expr;
    }
};

}
