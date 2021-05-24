#pragma once

#include <memory>
#include <utility>
#include <string>
#include <vector>

#include <fak/lang/token.h>

namespace fk::lang {

// forward declare our expression types for the visitor
struct binary_expression;
struct unary_expression;
struct literal_expression;
struct paren_expression;
struct identifier_expression;
struct and_expression;
struct or_expression;
struct call_expression;

template <class R>
struct expression_visitor {
    virtual ~expression_visitor() = default;

    virtual R visit(binary_expression *expr) = 0;
    virtual R visit(unary_expression *expr) = 0;
    virtual R visit(literal_expression *expr) = 0;
    virtual R visit(paren_expression *expr) = 0;
    virtual R visit(identifier_expression *expr) = 0;
    virtual R visit(and_expression *expr) = 0;
    virtual R visit(or_expression *expr) = 0;
    virtual R visit(call_expression *expr) = 0;
};

using number = double;

enum class expr_result_type {
    RT_EXIT_CODE,
    RT_STRING,
    RT_NUMBER,
    RT_BOOL,
    RT_NIL
};

std::string expr_result_type_str(expr_result_type type) noexcept;

struct expr_result {

    // TODO: unionize these fields. Right now, getting implicitly deleted destructor error because of union
    // and I don't want to block myself figuring it out right now.
    std::string str;
    number      n;
    bool        b;
    int         exit_code;

    expr_result_type type;

    static expr_result nil();
    static expr_result num(number n);
    static expr_result string(std::string s);
    static expr_result boolean(bool b);

    std::string stringify() const noexcept;
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
    Token op;
    expression_ptr right;

    binary_expression(expression_ptr left, Token op, expression_ptr right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    virtual expr_result accept(expression_visitor<expr_result> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct unary_expression 
    : public expression 
{
    Token op;
    expression_ptr right;

    unary_expression(Token op, expression_ptr right)
        : op(std::move(op)), right(std::move(right)) {}
    
    virtual expr_result accept(expression_visitor<expr_result> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct literal_expression 
    : public expression 
{
    Token literal;

    literal_expression(Token literal)
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
    Token name;

    identifier_expression(Token name)
        : name(std::move(name)) {}

    virtual expr_result accept(expression_visitor<expr_result> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct and_expression 
    : public expression 
{
    expression_ptr left, right;

    and_expression(expression_ptr left, expression_ptr right)
        : left(std::move(left)), right(std::move(right)) {}

    virtual expr_result accept(expression_visitor<expr_result> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct or_expression 
    : public expression 
{
    expression_ptr left, right;

    or_expression(expression_ptr left, expression_ptr right)
        : left(std::move(left)), right(std::move(right)) {}

    virtual expr_result accept(expression_visitor<expr_result> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct call_expression 
    : public expression 
{
    expression_ptr name;
    std::vector<expression_ptr> args;

    call_expression(expression_ptr name, std::vector<expression_ptr> args)
        : name(std::move(name)), args(std::move(args)) {}

    virtual expr_result accept(expression_visitor<expr_result> *visitor) override
    {
        return visitor->visit(this);
    }
};

}
