#pragma once

#include <memory>
#include <utility>
#include <string>
#include <vector>

#include <fak/lang/token.h>

namespace fk::lang {

// forward declare our expression types for the visitor
struct BinaryExpression;
struct UnaryExpression;
struct LiteralExpression;
struct ParenExpression;
struct IdentifierExpression;
struct CallExpression;
struct LambdaExpression;
struct CommandExpression;

struct Callable;

template <class R>
struct ExpressionVisitor {
    virtual ~ExpressionVisitor() = default;

    virtual R visit(BinaryExpression *expr) = 0;
    virtual R visit(UnaryExpression *expr) = 0;
    virtual R visit(LiteralExpression *expr) = 0;
    virtual R visit(ParenExpression *expr) = 0;
    virtual R visit(IdentifierExpression *expr) = 0;
    virtual R visit(CallExpression *expr) = 0;
    virtual R visit(LambdaExpression *expr) = 0;
    virtual R visit(CommandExpression *expr) = 0;
};

using Number = double;

enum class ExprResultType {
    RT_STRING,
    RT_NUMBER,
    RT_BOOL,
    RT_CALLABLE,
    RT_NIL
};

std::string expr_result_type_str(ExprResultType type) noexcept;

struct ExprResult {

    std::string str;
    union {
        Number      n;
        bool        b;
        Callable    *callable;
    };

    ExprResultType type;

    static ExprResult nil();
    static ExprResult num(Number n);
    static ExprResult string(std::string s);
    static ExprResult boolean(bool b);
    static ExprResult call(Callable *callable);

    std::string stringify() const noexcept;
};

struct Expression {
    virtual ~Expression() = default;

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) = 0;
};

using ExpressionPtr = std::unique_ptr<Expression>;

template <class T, class... Args>
ExpressionPtr make_expression(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

struct BinaryExpression 
    : public Expression 
{
    ExpressionPtr left;
    Token op;
    ExpressionPtr right;

    BinaryExpression(ExpressionPtr left, Token op, ExpressionPtr right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct UnaryExpression 
    : public Expression 
{
    Token op;
    ExpressionPtr right;

    UnaryExpression(Token op, ExpressionPtr right)
        : op(std::move(op)), right(std::move(right)) {}
    
    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct LiteralExpression 
    : public Expression 
{
    Token literal;

    LiteralExpression(Token literal)
        : literal(std::move(literal)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct ParenExpression 
    : public Expression 
{
    ExpressionPtr expr;

    ParenExpression(ExpressionPtr expr)
        : expr(std::move(expr)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct IdentifierExpression 
    : public Expression 
{
    Token name;

    IdentifierExpression(Token name)
        : name(std::move(name)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct CallExpression 
    : public Expression 
{
    ExpressionPtr callee;
    std::vector<ExpressionPtr> args;

    CallExpression(ExpressionPtr name, std::vector<ExpressionPtr> args)
        : callee(std::move(name)), args(std::move(args)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct CommandExpression 
    : public Expression 
{
    Token cmd;

    CommandExpression(Token cmd)
        : cmd(std::move(cmd)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override
    {
        return visitor->visit(this);
    }
};

}
