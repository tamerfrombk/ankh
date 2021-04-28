#pragma once

#include <vector>

#include <fak/lang/expr.h>
#include <fak/lang/statement.h>
#include <fak/lang/env.h>

namespace fk::lang {

class interpreter_t
    : public expression_visitor_t<expr_result_t>
    , public statement_visitor_t<void>
{
public:
    interpreter_t();

    void interpret(const program_t& program);

    virtual expr_result_t visit(binary_expression_t *expr) override;
    virtual expr_result_t visit(unary_expression_t *expr) override;
    virtual expr_result_t visit(literal_expression_t *expr) override;
    virtual expr_result_t visit(paren_expression_t *expr) override;
    virtual expr_result_t visit(identifier_expression_t *expr) override;

    virtual void visit(print_statement_t *stmt) override;
    virtual void visit(expression_statement_t *stmt) override;
    virtual void visit(assignment_statement_t *stmt) override;
    virtual void visit(block_statement_t *stmt) override;

private:
    expr_result_t evaluate(expression_ptr& expr) noexcept;
    void execute(const statement_ptr& stmt) noexcept;

    void enter_new_scope();
    void leave_current_scope();
    fk::lang::environment_t& current_scope();
    
private:
    std::vector<fk::lang::environment_t> env_;
};

}

