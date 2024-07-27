#pragma once

#include <ankh/lang/expr.hpp>
#include <ankh/lang/statement.hpp>

namespace ankh::lang {

// This is here instead of in lang/expr.hpp because we need both expression and statement constructs
// to create a lambda
// Including this in expr.hpp will create a circular include problem between expr.hpp and statement.hpp
// since statement.hpp relies on expr.hpp which would rely and statement.hpp and so on
struct LambdaExpression 
    : public Expression 
{
    Token marker;
    std::string generated_name;
    std::vector<Token> params;
    StatementPtr body;

    LambdaExpression(Token marker, std::string generated_name, std::vector<Token> params, StatementPtr body)
        : marker(std::move(marker)), generated_name(std::move(generated_name)), params(std::move(params)), body(std::move(body)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override
    {
        return visitor->visit(this);
    }

    virtual std::string stringify() const noexcept override
    {
        std::string params = "";
        return "fn (" + params + ") " + body->stringify();
    }
};

}