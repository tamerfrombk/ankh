#pragma once

#include <clean/lang/expr.h>
#include <clean/lang/statement.h>
#include <clean/lang/env.h>

class interpreter_t
    : public expression_visitor_t<expr_result_t>
    , public statement_visitor_t<void>
{
public:
    void interpret(const program_t& program);

    virtual expr_result_t visit(binary_expression_t *expr) override;
    virtual expr_result_t visit(unary_expression_t *expr) override;
    virtual expr_result_t visit(literal_expression_t *expr) override;
    virtual expr_result_t visit(paren_expression_t *expr) override;
    virtual expr_result_t visit(identifier_expression_t *expr) override;

    virtual void visit(print_statement_t *stmt) override;
    virtual void visit(expression_statement_t *stmt) override;
    virtual void visit(assignment_statement_t *stmt) override;

private:
    expr_result_t evaluate(expression_ptr& expr) noexcept;
    void execute(const statement_ptr& stmt) noexcept;
    
private:
    environment_t env_;
};
