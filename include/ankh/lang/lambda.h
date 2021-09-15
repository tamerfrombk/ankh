#pragma once

#include <ankh/lang/expr.h>
#include <ankh/lang/statement.h>

namespace ankh::lang {

// This is here instead of in lang/expr.h because we need both expression and statement constructs
// to create a lambda
// Including this in expr.h will create a circular include problem between expr.h and statement.h
// since statement.h relies on expr.h which would rely and statement.h and so on
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