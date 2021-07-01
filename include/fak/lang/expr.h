#pragma once

#include "fak/log.h"
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
struct ArrayExpression;
struct IndexExpression;

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
    virtual R visit(ArrayExpression *expr) = 0;
    virtual R visit(IndexExpression *expr) = 0;
};

using Number = double;

enum class ExprResultType {
    RT_STRING,
    RT_NUMBER,
    RT_BOOL,
    RT_CALLABLE,
    RT_ARRAY,
    RT_NIL
};

std::string expr_result_type_str(ExprResultType type) noexcept;

struct ExprResult;

using ArrayType = std::vector<fk::lang::ExprResult>;
class Array
{
public:
    Array()                : elems_(std::make_shared<ArrayType>()) {}
    Array(ArrayType elems) : elems_(std::make_shared<ArrayType>(std::move(elems))) {}

    void append(const ExprResult& result) noexcept
    {
        elems_->push_back(result);
    }

    bool empty() const noexcept
    {
        return elems_->empty();
    }

    fk::lang::ExprResult& operator[](size_t i) noexcept
    {
        return (*elems_)[i];
    }

    const fk::lang::ExprResult& operator[](size_t i) const noexcept
    {
        return (*elems_)[i];
    }

    size_t size() const noexcept
    {
        return elems_->size();
    }

    friend bool operator==(const Array& lhs, const Array& rhs) noexcept
    {
        return *lhs.elems_ == *rhs.elems_;
    }

    friend bool operator!=(const Array& lhs, const Array& rhs) noexcept
    {
        return !(operator==(lhs, rhs));
    }

private:
    std::shared_ptr<ArrayType> elems_;
};


struct ExprResult {

    std::string str;
    union {
        Number      n;
        bool        b;
        Callable    *callable;
    };
    Array array;
    ExprResultType type;

    ExprResult()                   :                      type(ExprResultType::RT_NIL) {}
    ExprResult(std::string str)    : str(std::move(str)), type(ExprResultType::RT_STRING) {}
    ExprResult(Number n)           : n(n)               , type(ExprResultType::RT_NUMBER) {}
    ExprResult(bool b)             : b(b)               , type(ExprResultType::RT_BOOL) {}
    ExprResult(Callable *callable) : callable(callable) , type(ExprResultType::RT_CALLABLE) {}
    ExprResult(Array array)        : array(array)       , type(ExprResultType::RT_ARRAY) {}

    std::string stringify() const noexcept;

    friend bool operator==(const ExprResult& lhs, const ExprResult& rhs) noexcept
    {
        if (lhs.type != rhs.type) {
            return false;
        }

        switch (lhs.type)
        {
        case ExprResultType::RT_NIL:      return true;
        case ExprResultType::RT_STRING:   return lhs.str == rhs.str;
        case ExprResultType::RT_NUMBER:   return lhs.n == rhs.n;
        case ExprResultType::RT_BOOL:     return lhs.b == rhs.b;
        case ExprResultType::RT_CALLABLE: return lhs.callable == rhs.callable;
        case ExprResultType::RT_ARRAY:    return lhs.array == rhs.array;
        default: FK_FATAL("unknown expression result type");
        }
    }

    friend bool operator!=(const ExprResult& lhs, const ExprResult& rhs) noexcept
    {
        return !(operator==(lhs, rhs));
    }
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

}
