#pragma once

#include <fak/lang/expr.h>
#include <fak/lang/statement.h>

namespace fk::lang {

// TODO: this is here instead of in lang/expr.h because we need both expression and statement constructs
// to create a lambda and including this in expr.h will create a circular include problem between expr.h and statement.h
// since statement.h relies on expr.h

struct LambdaExpression 
    : public Expression 
{
    std::string generated_name;
    std::vector<Token> params;
    StatementPtr body;

    LambdaExpression(std::string generated_name, std::vector<Token> params, StatementPtr body)
        : generated_name(std::move(generated_name)), params(std::move(params)), body(std::move(body)) {}

    virtual ExprResult accept(ExpressionVisitor<ExprResult> *visitor) override
    {
        return visitor->visit(this);
    }
};

}