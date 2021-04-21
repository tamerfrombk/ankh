#pragma once

#include <memory>
#include <utility>
#include <string>

#include <clean/lang/token.h>


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

using number_t = double;

enum class expr_result_type {
    RT_EXIT_CODE,
    RT_STRING,
    RT_NUMBER,
    RT_NIL,
    RT_ERROR
};

std::string expr_result_type_str(expr_result_type type) noexcept;

struct expr_result_t {

    // TODO: unionize these fields. Right now, getting implicitly deleted destructor error because of union
    // and I don't want to block myself figuring it out right now.
    std::string str;
    std::string err;
    number_t    n;
    int         exit_code;

    expr_result_type type;

    static expr_result_t nil();
    static expr_result_t num(number_t n);
    static expr_result_t stringe(std::string s);
    static expr_result_t e(std::string s);
};

struct expression_t {
    virtual ~expression_t() = default;

    virtual expr_result_t accept(expression_visitor_t<expr_result_t> *visitor) = 0;
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

    virtual expr_result_t accept(expression_visitor_t<expr_result_t> *visitor) override
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
    
    virtual expr_result_t accept(expression_visitor_t<expr_result_t> *visitor) override
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

    virtual expr_result_t accept(expression_visitor_t<expr_result_t> *visitor) override
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

    virtual expr_result_t accept(expression_visitor_t<expr_result_t> *visitor) override
    {
        return visitor->visit(this);
    }
};
