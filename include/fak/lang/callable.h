#pragma once

#include <string>
#include <vector>
#include <memory>

#include <fak/lang/expr.h>
#include <fak/lang/statement.h>
#include <fak/lang/lambda.h>

#include <fak/lang/env.h>

#include <fak/log.h>

namespace fk::lang {

struct Callable
{
    virtual ~Callable() = default;

    virtual std::string name() const noexcept = 0;

    virtual size_t arity() const noexcept = 0;

    virtual void invoke(const std::vector<ExpressionPtr>& args) = 0;
};

using CallablePtr = std::unique_ptr<Callable>;

template <class T, class... Args>
CallablePtr make_callable(Args&&... args) noexcept
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <class T, class I>
class Function
    : public Callable
{
public:
    Function(I* interpreter, StatementPtr stmt, EnvironmentPtr<T> closure)
        : interpreter_(interpreter)
        , stmt_(std::move(stmt))
        , decl_(static_cast<FunctionDeclaration*>(stmt_.get()))
        , closure_(closure) {}
    
    virtual std::string name() const noexcept override
    {
        return decl_->name.str;
    }

    virtual size_t arity() const noexcept override
    {
        return decl_->params.size();
    }

    virtual void invoke(const std::vector<ExpressionPtr>& args) override
    {
        EnvironmentPtr<T> environment(make_env<T>(closure_));
        FK_DEBUG("closure environment {} created", environment->scope());
        for (size_t i = 0; i < args.size(); ++i) {
            const ExprResult arg = interpreter_->evaluate(args[i]);
            if (!environment->declare(decl_->params[i].str, arg)) {
                FK_FATAL("function parameter '{}' should always be declarable");
            }
        }

        BlockStatement* block = static_cast<BlockStatement*>(decl_->body.get());
        interpreter_->execute_block(block, environment);
    }

private:
    I *interpreter_;
    StatementPtr stmt_;
    FunctionDeclaration *decl_;
    EnvironmentPtr<T> closure_;
};

template <class T, class I>
class Lambda
    : public Callable
{
public:
    Lambda(I* interpreter, ExpressionPtr expr, EnvironmentPtr<T> closure)
        : interpreter_(interpreter)
        , expr_(std::move(expr))
        , lambda_(static_cast<LambdaExpression*>(expr_.get()))
        , closure_(closure) {}
    
    virtual std::string name() const noexcept override
    {
        return lambda_->generated_name;
    }

    virtual size_t arity() const noexcept override
    {
        return lambda_->params.size();
    }

    virtual void invoke(const std::vector<ExpressionPtr>& args) override
    {
        EnvironmentPtr<T> environment(make_env<T>(closure_));
        FK_DEBUG("closure environment {} created", environment->scope());
        for (size_t i = 0; i < args.size(); ++i) {
            const ExprResult arg = interpreter_->evaluate(args[i]);
            if (!environment->declare(lambda_->params[i].str, arg)) {
                FK_FATAL("function parameter '{}' should always be declarable");
            }
        }

        BlockStatement* block = static_cast<BlockStatement*>(lambda_->body.get());
        interpreter_->execute_block(block, environment);
    }

private:
    I *interpreter_;
    ExpressionPtr expr_;
    LambdaExpression *lambda_;
    EnvironmentPtr<T> closure_;
};

}