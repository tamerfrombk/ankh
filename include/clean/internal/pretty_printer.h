#pragma once

#include <initializer_list>
#include <string>

#include <clean/lang/expr.h>
#include <clean/lang/token.h>
#include <clean/log.h>

class pretty_printer_t
    : public expression_visitor_t<expr_result_t>
{
public:
    inline virtual expr_result_t visit(binary_expression_t *expr) override
    {
        return paren(expr->op.str, { expr->left.get(), expr->right.get() });
    }

    inline virtual expr_result_t visit(unary_expression_t *expr) override
    {
        return paren(expr->op.str, { expr->right.get() });
    }

    inline virtual expr_result_t visit(literal_expression_t *expr) override
    {
        return paren(expr->literal.str, {});
    }

    inline virtual expr_result_t visit(paren_expression_t *expr) override
    {
        return paren("paren", { expr->expr.get() });
    }

private:
    inline expr_result_t paren(const std::string& op, std::initializer_list<expression_t*> exprs)
    {
        std::string result;

        result += "( ";
        result += op;
        for (const auto expr : exprs) {
            result += " ";
            result += expr->accept(this).str;
        }
        result += " )";

        expr_result_t expr;
        expr.type = expr_result_type::RT_STRING;
        expr.str  = result;

        return expr;
    }
};