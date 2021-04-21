#pragma once

#include <cstdlib>

#include <clean/lang/token.h>
#include <clean/lang/expr.h>
#include <clean/log.h>

class interpreter_t
    : public expression_visitor_t<expr_result_t>
{
public:
    virtual expr_result_t visit(binary_expression_t *expr) override
    {
        const expr_result_t left  = expr->left->accept(this);
        const expr_result_t right = expr->right->accept(this);

        // TODO: implement other binary operators
        switch (expr->op.type) {
        case token_type::PLUS:
            if (left.type == expr_result_type::RT_NUMBER && right.type == expr_result_type::RT_NUMBER) {
                return expr_result_t::num(left.n + right.n);
            }
            if (left.type == expr_result_type::RT_STRING && right.type == expr_result_type::RT_STRING) {
                return expr_result_t::stringe(left.str + right.str);
            }
            return expr_result_t::e("unknown overload for (+) operator ");
        default:
            fatal("unimplemented binary operator '%s'\n", expr->op.str.c_str());
        }
    }

    virtual expr_result_t visit(unary_expression_t *expr) override
    {
        const expr_result_t result = expr->right->accept(this);

        // TODO: support '!' boolean
        if (expr->op.type == token_type::MINUS) {
            return negate(result);
        }

        // At this point, the parser is broken so bail out
        fatal("unsupported unary expression operator '%s'\n", expr->op.str.c_str());
    }

    virtual expr_result_t visit(literal_expression_t *expr) override
    {
        switch (expr->literal.type) {
        case token_type::NUMBER:
            return expr_result_t::num(to_num(expr->literal.str));
        case token_type::STRING:
            return expr_result_t::stringe(expr->literal.str);
        default:
            // TODO: handle error better than this
            return expr_result_t::e("unknown literal type");
        }
    }

    virtual expr_result_t visit(paren_expression_t *expr) override
    {
        return expr->expr->accept(this);
    }

private:
    number_t to_num(const std::string& s)
    {
        char *end;
        number_t n = std::strtod(s.c_str(), &end);
        if (*end == '\0') {
            return n;
        }

        // TODO: properly handle error
        return -1;
    }

    expr_result_t negate(const expr_result_t& result)
    {
        if (result.type == expr_result_type::RT_NUMBER) {
            return expr_result_t::num(-1 * result.n);
        }
        
        // TODO: improve the error message
        return expr_result_t::e("- operator expects a number expression");
    }
};