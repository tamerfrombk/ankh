#pragma once

#include "clean/clean.h"
#include <memory>
#include <utility>
#include <clean/token.h>


// forward declare our expression types for the visitor
struct binary_expression_t;
struct unary_expression_t;
struct literal_expression_t;
struct paren_expression_t;

template <class R>
struct expression_visitor_t {
    virtual ~expression_visitor_t() = default;

    virtual R visit(binary_expression_t *expr) = 0;
    virtual R visit(unary_expression_t *expr) = 0;
    virtual R visit(literal_expression_t *expr) = 0;
    virtual R visit(paren_expression_t *expr) = 0;
};

struct expression_t {
    virtual ~expression_t() = default;

    virtual std::string accept(expression_visitor_t<std::string> *visitor) = 0;
};

using expression_ptr = std::unique_ptr<expression_t>;

template <class T, class... Args>
expression_ptr make_expression(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

struct binary_expression_t 
    : public expression_t 
{
    expression_ptr left;
    token_t op;
    expression_ptr right;

    binary_expression_t(expression_ptr left, token_t op, expression_ptr right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    virtual std::string accept(expression_visitor_t<std::string> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct unary_expression_t 
    : public expression_t 
{
    token_t op;
    expression_ptr right;

    unary_expression_t(token_t op, expression_ptr right)
        : op(std::move(op)), right(std::move(right)) {}
    
    virtual std::string accept(expression_visitor_t<std::string> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct literal_expression_t 
    : public expression_t 
{
    token_t literal;

    literal_expression_t(token_t literal)
        : literal(std::move(literal)) {}

    virtual std::string accept(expression_visitor_t<std::string> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct paren_expression_t 
    : public expression_t 
{
    expression_ptr expr;

    paren_expression_t(expression_ptr expr)
        : expr(std::move(expr)) {}

    virtual std::string accept(expression_visitor_t<std::string> *visitor) override
    {
        return visitor->visit(this);
    }
};
