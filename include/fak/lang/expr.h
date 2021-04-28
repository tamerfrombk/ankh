#pragma once

#include <memory>
#include <utility>
#include <string>

#include <fak/lang/token.h>

namespace fk::lang {

// forward declare our expression types for the visitor
struct binary_expression;
struct unary_expression;
struct literal_expression;
struct paren_expression;
struct identifier_expression;

template <class R>
struct expression_visitor {
    virtual ~expression_visitor() = default;

    virtual R visit(binary_expression *expr) = 0;
    virtual R visit(unary_expression *expr) = 0;
    virtual R visit(literal_expression *expr) = 0;
    virtual R visit(paren_expression *expr) = 0;
    virtual R visit(identifier_expression *expr) = 0;
};

using number_t = double;

enum class expr_result_type {
    RT_EXIT_CODE,
    RT_STRING,
    RT_NUMBER,
    RT_BOOL,
    RT_NIL,
    RT_ERROR
};

std::string expr_result_type_str(expr_result_type type) noexcept;

struct expr_result {

    // TODO: unionize these fields. Right now, getting implicitly deleted destructor error because of union
    // and I don't want to block myself figuring it out right now.
    std::string str;
    std::string err;
    number_t    n;
    bool        b;
    int         exit_code;

    expr_result_type type;

    static expr_result nil();
    static expr_result num(number_t n);
    static expr_result stringe(std::string s);
    static expr_result e(std::string s);
    static expr_result boolean(bool b);
};

struct expression {
    virtual ~expression() = default;

    virtual expr_result accept(expression_visitor<expr_result> *visitor) = 0;
};

using expression_ptr = std::unique_ptr<expression>;

template <class T, class... Args>
expression_ptr make_expression(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

struct binary_expression 
    : public expression 
{
    expression_ptr left;
    token op;
    expression_ptr right;

    binary_expression(expression_ptr left, token op, expression_ptr right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    virtual expr_result accept(expression_visitor<expr_result> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct unary_expression 
    : public expression 
{
    token op;
    expression_ptr right;

    unary_expression(token op, expression_ptr right)
        : op(std::move(op)), right(std::move(right)) {}
    
    virtual expr_result accept(expression_visitor<expr_result> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct literal_expression 
    : public expression 
{
    token literal;

    literal_expression(token literal)
        : literal(std::move(literal)) {}

    virtual expr_result accept(expression_visitor<expr_result> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct paren_expression 
    : public expression 
{
    expression_ptr expr;

    paren_expression(expression_ptr expr)
        : expr(std::move(expr)) {}

    virtual expr_result accept(expression_visitor<expr_result> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct identifier_expression 
    : public expression 
{
    token name;

    identifier_expression(token name)
        : name(std::move(name)) {}

    virtual expr_result accept(expression_visitor<expr_result> *visitor) override
    {
        return visitor->visit(this);
    }
};

}
