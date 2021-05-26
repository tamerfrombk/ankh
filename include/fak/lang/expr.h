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
struct AndExpression;
struct OrExpression;
struct CallExpression;

template <class R>
struct ExpressionVisitor {
    virtual ~ExpressionVisitor() = default;

    virtual R visit(BinaryExpression *expr) = 0;
    virtual R visit(UnaryExpression *expr) = 0;
    virtual R visit(LiteralExpression *expr) = 0;
    virtual R visit(ParenExpression *expr) = 0;
    virtual R visit(IdentifierExpression *expr) = 0;
    virtual R visit(AndExpression *expr) = 0;
    virtual R visit(OrExpression *expr) = 0;
    virtual R visit(CallExpression *expr) = 0;
};

using Number = double;

enum class ExprResultType {
    RT_EXIT_CODE,
    RT_STRING,
    RT_NUMBER,
    RT_BOOL,
    RT_NIL
};

std::string expr_result_type_str(ExprResultType type) noexcept;

struct ExprResult {

    // TODO: unionize these fields. Right now, getting implicitly deleted destructor error because of union
    // and I don't want to block myself figuring it out right now.
    std::string str;
    Number      n;
    bool        b;
    int         exit_code;

    ExprResultType type;

    static ExprResult nil();
    static ExprResult num(Number n);
    static ExprResult string(std::string s);
    static ExprResult boolean(bool b);

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

struct AndExpression 
    : public Expression 
{
    ExpressionPtr left, right;

    AndExpression(ExpressionPtr left, ExpressionPtr right)
        : left(std::move(left)), right(std::move(right)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct OrExpression 
    : public Expression 
{
    ExpressionPtr left, right;

    OrExpression(ExpressionPtr left, ExpressionPtr right)
        : left(std::move(left)), right(std::move(right)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct CallExpression 
    : public Expression 
{
    ExpressionPtr name;
    std::vector<ExpressionPtr> args;

    CallExpression(ExpressionPtr name, std::vector<ExpressionPtr> args)
        : name(std::move(name)), args(std::move(args)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override
    {
        return visitor->visit(this);
    }
};

}
