#pragma once

#include <initializer_list>
#include <string>

#include <fak/lang/expr.h>
#include <fak/lang/token.h>
#include <fak/log.h>

namespace fk::internal {

class pretty_printer_t
    : public fk::lang::expression_visitor_t<fk::lang::expr_result_t>
{
public:
    inline virtual fk::lang::expr_result_t visit(fk::lang::binary_expression_t *expr) override
    {
        return paren(expr->op.str, { expr->left.get(), expr->right.get() });
    }

    inline virtual fk::lang::expr_result_t visit(fk::lang::unary_expression_t *expr) override
    {
        return paren(expr->op.str, { expr->right.get() });
    }

    inline virtual fk::lang::expr_result_t visit(fk::lang::literal_expression_t *expr) override
    {
        return paren(expr->literal.str, {});
    }

    inline virtual fk::lang::expr_result_t visit(fk::lang::paren_expression_t *expr) override
    {
        return paren("paren", { expr->expr.get() });
    }

private:
    inline fk::lang::expr_result_t paren(const std::string& op, std::initializer_list<fk::lang::expression_t*> exprs)
    {
        std::string result;

        result += "( ";
        result += op;
        for (const auto expr : exprs) {
            result += " ";
            result += expr->accept(this).str;
        }
        result += " )";

        fk::lang::expr_result_t expr;
        expr.type = fk::lang::expr_result_type::RT_STRING;
        expr.str  = result;

        return expr;
    }
};

}
