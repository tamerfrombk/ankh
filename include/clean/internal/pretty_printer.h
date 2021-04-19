#pragma once

#include <initializer_list>
#include <string>

#include <clean/expr.h>
#include <clean/token.h>
#include <clean/log.h>

class pretty_printer_t
    : public expression_visitor_t<std::string>
{
public:
    inline virtual std::string visit(binary_expression_t *expr) override
    {
        return paren(expr->op.str, { expr->left.get(), expr->right.get() });
    }

    inline virtual std::string visit(unary_expression_t *expr) override
    {
        return paren(expr->op.str, { expr->right.get() });
    }

    inline virtual std::string visit(literal_expression_t *expr) override
    {
        return paren(expr->literal.str, {});
    }

    inline virtual std::string visit(paren_expression_t *expr) override
    {
        return paren("paren", { expr->expr.get() });
    }

private:
    inline std::string paren(const std::string& op, std::initializer_list<expression_t*> exprs)
    {
        std::string result;

        result += "( ";
        result += op;
        for (const auto expr : exprs) {
            result += " ";
            result += expr->accept(this);
        }
        result += " )";

        return result;
    }
};