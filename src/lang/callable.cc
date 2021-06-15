#include <fak/lang/callable.h>

#include <fak/lang/interpreter.h>

#include <fak/log.h>

fk::lang::Function::Function(Interpreter *interpreter, FunctionDeclaration *decl, EnvironmentPtr closure)
    : interpreter_(interpreter)
    , decl_(decl)
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
    Environment environment(closure_.get());
    FK_DEBUG("closure environment {} created", environment.scope());
    for (size_t i = 0; i < args.size(); ++i) {
        const ExprResult arg = interpreter_->evaluate(args[i]);
        if (!environment.declare(decl_->params[i].str, arg)) {
            FK_FATAL("function parameter '{}' should always be declarable");
        }
    }

    BlockStatement *block = static_cast<BlockStatement*>(decl_->body.get());
    interpreter_->execute_block(block, &environment);
}

fk::lang::Lambda::Lambda(Interpreter *interpreter, LambdaExpression *decl, EnvironmentPtr closure)
    : interpreter_(interpreter)
    , decl_(decl)
    , closure_(closure)
{}

std::string fk::lang::Lambda::name() const noexcept
{
    return decl_->generated_name;
}

size_t fk::lang::Lambda::arity() const noexcept
{
    return decl_->params.size();
}

void fk::lang::Lambda::invoke(const std::vector<ExpressionPtr>& args)
{
    Environment environment(closure_.get());
    FK_DEBUG("closure environment {} created", environment.scope());
    for (size_t i = 0; i < args.size(); ++i) {
        const ExprResult arg = interpreter_->evaluate(args[i]);
        if (!environment.declare(decl_->params[i].str, arg)) {
            FK_FATAL("function parameter '{}' should always be declarable");
        }
    }

    BlockStatement *block = static_cast<BlockStatement*>(decl_->body.get());
    interpreter_->execute_block(block, &environment);
}