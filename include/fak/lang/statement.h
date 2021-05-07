#pragma once

#include <memory>
#include <utility>
#include <vector>

#include <fak/lang/expr.h>
#include <fak/lang/token.h>

namespace fk::lang {

// forward declare statement types for visitor
struct print_statement;
struct expression_statement;
struct variable_declaration;
struct assignment_statement;
struct block_statement;
struct if_statement;
struct while_statement;
struct function_declaration;

template <class R>
struct statement_visitor {
    virtual ~statement_visitor() = default;

    virtual R visit(print_statement *stmt) = 0;
    virtual R visit(expression_statement *stmt) = 0;
    virtual R visit(variable_declaration *stmt) = 0;
    virtual R visit(assignment_statement *stmt) = 0;
    virtual R visit(block_statement *stmt) = 0;
    virtual R visit(if_statement *stmt) = 0;
    virtual R visit(while_statement *stmt) = 0;
    virtual R visit(function_declaration *stmt) = 0;
};

struct statement
{
    virtual ~statement() = default;

    virtual void accept(statement_visitor<void> *visitor) = 0;
    virtual std::string accept(statement_visitor<std::string>  *visitor) = 0;
};

using statement_ptr = std::unique_ptr<statement>;
using program     = std::vector<statement_ptr>;

template <class T, class... Args>
statement_ptr make_statement(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

struct print_statement
    : public statement
{
    expression_ptr expr;

    print_statement(expression_ptr expr)
        : expr(std::move(expr)) {}

    virtual void accept(statement_visitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(statement_visitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct expression_statement
    : public statement
{
    expression_ptr expr;

    expression_statement(expression_ptr expr)
        : expr(std::move(expr)) {}

    virtual void accept(statement_visitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(statement_visitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct assignment_statement
    : public statement
{
    token name;
    expression_ptr initializer;

    assignment_statement(token name, expression_ptr initializer)
        : name(std::move(name)), initializer(std::move(initializer)) {}

    virtual void accept(statement_visitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(statement_visitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct variable_declaration
    : public statement
{
    token name;
    expression_ptr initializer;

    variable_declaration(token name, expression_ptr initializer)
        : name(std::move(name)), initializer(std::move(initializer)) {}

    virtual void accept(statement_visitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(statement_visitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct block_statement
    : public statement
{
    std::vector<statement_ptr> statements;

    block_statement(std::vector<statement_ptr> statements)
        : statements(std::move(statements)) {}

    virtual void accept(statement_visitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(statement_visitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct if_statement
    : public statement
{
    expression_ptr condition;
    statement_ptr then_block;
    statement_ptr else_block;

    if_statement(expression_ptr condition, statement_ptr then_block, statement_ptr else_block)
        : condition(std::move(condition)), then_block(std::move(then_block)), else_block(std::move(else_block)) {}

    virtual void accept(statement_visitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(statement_visitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct while_statement
    : public statement
{
    expression_ptr condition;
    statement_ptr body;

    while_statement(expression_ptr condition, statement_ptr body)
        : condition(std::move(condition)), body(std::move(body)) {}

    virtual void accept(statement_visitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(statement_visitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct function_declaration
    : public statement
{
    token name;
    std::vector<token> params;
    statement_ptr body;

    function_declaration(token name, std::vector<token> params, statement_ptr body)
        : name(std::move(name)), params(std::move(params)), body(std::move(body)) {}

    virtual void accept(statement_visitor<void> *visitor) override
    {
        visitor->visit(this);
    }

    virtual std::string accept(statement_visitor<std::string> *visitor) override
    {
        return visitor->visit(this);
    }
};

}
