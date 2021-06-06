#include <fak/lang/callable.h>

#include <fak/lang/interpreter.h>

fk::lang::Function::Function(Interpreter *interpreter, FunctionDeclaration *decl)
    : interpreter_(interpreter)
    , decl_(decl)
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
    for (size_t i = 0; i < args.size(); ++i) {
        const ExprResult arg = interpreter_->evaluate(args[i]);
        interpreter_->current_scope().assign(decl_->params[i].str, arg);
    }

    // Here, we avoid calling execute(declaration->body) directly because visiting a block statement will
    // create a new additional environment we don't want. When we implement function calls, we will be controlling
    // the environment of the child block so we execute its statements directly here.
    BlockStatement *block = static_cast<BlockStatement*>(decl_->body.get());
    for (const auto& stmt : block->statements) {
        interpreter_->execute(stmt);
    }
}