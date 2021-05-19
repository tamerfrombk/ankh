#pragma once

#include <vector>

#include <fak/lang/expr.h>
#include <fak/lang/statement.h>
#include <fak/lang/env.h>

namespace fk::lang {

class interpreter
    : public expression_visitor<expr_result>
    , public statement_visitor<void>
{
public:
    interpreter();

    void interpret(const program& program);

    virtual expr_result visit(binary_expression *expr) override;
    virtual expr_result visit(unary_expression *expr) override;
    virtual expr_result visit(literal_expression *expr) override;
    virtual expr_result visit(paren_expression *expr) override;
    virtual expr_result visit(identifier_expression *expr) override;
    virtual expr_result visit(and_expression *expr) override;
    virtual expr_result visit(or_expression *expr) override;

    virtual void visit(print_statement *stmt) override;
    virtual void visit(expression_statement *stmt) override;
    virtual void visit(variable_declaration *stmt) override;
    virtual void visit(assignment_statement *stmt) override;
    virtual void visit(block_statement *stmt) override;
    virtual void visit(if_statement *stmt) override;
    virtual void visit(while_statement *stmt) override;

private:
    expr_result evaluate(expression_ptr& expr);
    void execute(const statement_ptr& stmt);

    void enter_new_scope() noexcept;
    void leave_current_scope() noexcept;
    environment& current_scope() noexcept;
    
private:
    std::vector<environment> env_;
};

}

