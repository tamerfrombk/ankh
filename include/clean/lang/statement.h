#pragma once

#include <memory>
#include <utility>
#include <vector>

#include <clean/lang/expr.h>
#include <clean/lang/token.h>

// forward declare statement types for visitor
struct print_statement_t;
struct expression_statement_t;
struct assignment_statement_t;

template <class R>
struct statement_visitor_t {
    virtual ~statement_visitor_t() = default;

    virtual R visit(print_statement_t *stmt) = 0;
    virtual R visit(expression_statement_t *stmt) = 0;
    virtual R visit(assignment_statement_t *stmt) = 0;
};

struct statement_t
{
    virtual ~statement_t() = default;

    virtual void accept(statement_visitor_t<void> *visitor) = 0;
};

using statement_ptr = std::unique_ptr<statement_t>;
using program_t     = std::vector<statement_ptr>;

template <class T, class... Args>
statement_ptr make_statement(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

struct print_statement_t
    : public statement_t
{
    expression_ptr expr;

    print_statement_t(expression_ptr expr)
        : expr(std::move(expr)) {}

    virtual void accept(statement_visitor_t<void> *visitor) override
    {
        visitor->visit(this);
    }
};

struct expression_statement_t
    : public statement_t
{
    expression_ptr expr;

    expression_statement_t(expression_ptr expr)
        : expr(std::move(expr)) {}

    virtual void accept(statement_visitor_t<void> *visitor) override
    {
        visitor->visit(this);
    }
};

struct assignment_statement_t
    : public statement_t
{
    token_t name;
    expression_ptr initializer;

    assignment_statement_t(token_t name, expression_ptr initializer)
        : name(std::move(name)), initializer(std::move(initializer)) {}

    virtual void accept(statement_visitor_t<void> *visitor) override
    {
        visitor->visit(this);
    }
};