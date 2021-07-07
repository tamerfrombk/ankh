#pragma once

#include <memory>
#include <utility>
#include <string>
#include <vector>

#include <fak/lang/token.h>
#include <fak/lang/expr_result.h>

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
struct ArrayExpression;
struct IndexExpression;
struct DictionaryExpression;
struct StringExpression;


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
    virtual R visit(ArrayExpression *expr) = 0;
    virtual R visit(IndexExpression *expr) = 0;
    virtual R visit(DictionaryExpression *expr) = 0;
    virtual R visit(StringExpression *expr) = 0;
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

struct StringExpression 
    : public Expression 
{
    Token str;

    StringExpression(Token str)
        : str(std::move(str)) {}

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

struct ArrayExpression 
    : public Expression 
{
    std::vector<ExpressionPtr> elems;

    ArrayExpression(std::vector<ExpressionPtr> elems)
        : elems(std::move(elems)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override
    {
        return visitor->visit(this);
    }

    size_t size() const noexcept
    {
        return elems.size();
    }
};

struct IndexExpression 
    : public Expression 
{
    ExpressionPtr indexee;
    ExpressionPtr index;

    IndexExpression(ExpressionPtr indexee, ExpressionPtr index)
        : indexee(std::move(indexee)), index(std::move(index)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override
    {
        return visitor->visit(this);
    }
};

struct DictionaryExpression 
    : public Expression
{
    std::vector<Entry<ExpressionPtr>> entries;

    DictionaryExpression(std::vector<Entry<ExpressionPtr>> entries)
        : entries(std::move(entries)) {}
    
    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override
    {
        return visitor->visit(this);
    }
};

}
