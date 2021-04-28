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
struct assignment_statement;
struct block_statement;

template <class R>
struct statement_visitor {
    virtual ~statement_visitor() = default;

    virtual R visit(print_statement *stmt) = 0;
    virtual R visit(expression_statement *stmt) = 0;
    virtual R visit(assignment_statement *stmt) = 0;
    virtual R visit(block_statement *stmt) = 0;
};

struct statement
{
    virtual ~statement() = default;

    virtual void accept(statement_visitor<void> *visitor) = 0;
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
};

}
