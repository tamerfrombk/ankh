#include <fak/lang/callable.h>

#include <fak/lang/interpreter.h>

#include <fak/log.h>

fk::lang::Function::Function(Interpreter *interpreter, StatementPtr stmt, EnvironmentPtr closure)
    : interpreter_(interpreter)
    , stmt_(std::move(stmt))
    , decl_(static_cast<FunctionDeclaration*>(stmt_.get()))
    , closure_(closure)
{}
    
std::string fk::lang::Function::name() const noexcept
{
    return "<fn " + decl_->name.str + ">";
}

size_t fk::lang::Function::arity() const noexcept
{
    return decl_->params.size();
}

void fk::lang::Function::invoke(const std::vector<ExpressionPtr>& args)
{
    EnvironmentPtr environment(make_env(closure_));
    FK_DEBUG("closure environment {} created", environment->scope());
    for (size_t i = 0; i < args.size(); ++i) {
        const ExprResult arg = interpreter_->evaluate(args[i]);
        if (!environment->declare(decl_->params[i].str, arg)) {
            FK_FATAL("function parameter '{}' should always be declarable");
        }
    }

    BlockStatement *block = static_cast<BlockStatement*>(decl_->body.get());
    interpreter_->execute_block(block, environment);
}

fk::lang::Lambda::Lambda(Interpreter *interpreter, ExpressionPtr expr, EnvironmentPtr closure)
    : interpreter_(interpreter)
    , expr_(std::move(expr))
    , lambda_(static_cast<LambdaExpression*>(expr_.get()))
    , closure_(closure)
{}

std::string fk::lang::Lambda::name() const noexcept
{
    return lambda_->generated_name;
}

size_t fk::lang::Lambda::arity() const noexcept
{
    return lambda_->params.size();
}

void fk::lang::Lambda::invoke(const std::vector<ExpressionPtr>& args)
{
    EnvironmentPtr environment(make_env(closure_));
    FK_DEBUG("closure environment {} created", environment->scope());
    for (size_t i = 0; i < args.size(); ++i) {
        const ExprResult arg = interpreter_->evaluate(args[i]);
        if (!environment->declare(lambda_->params[i].str, arg)) {
            FK_FATAL("function parameter '{}' should always be declarable");
        }
    }

    BlockStatement *block = static_cast<BlockStatement*>(lambda_->body.get());
    interpreter_->execute_block(block, environment);
}