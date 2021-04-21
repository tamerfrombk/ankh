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
    number_t      to_num(const std::string& s) const noexcept;
    expr_result_t negate(const expr_result_t& result) const noexcept;
    expr_result_t invert(const expr_result_t& result) const noexcept;
};
