#pragma once

#include <clean/lang/expr.h>

class interpreter_t
    : public expression_visitor_t<expr_result_t>
{
public:
    virtual expr_result_t visit(binary_expression_t *expr) override;
    virtual expr_result_t visit(unary_expression_t *expr) override;
    virtual expr_result_t visit(literal_expression_t *expr) override;
    virtual expr_result_t visit(paren_expression_t *expr) override;
private:
    expr_result_t evaluate(expression_ptr& expr) noexcept;
};
