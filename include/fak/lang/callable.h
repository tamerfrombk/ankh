#pragma once

#include <string>
#include <vector>
#include <memory>

#include <fak/lang/expr.h>
#include <fak/lang/statement.h>


namespace fk::lang {

    class Interpreter;

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

class Function
    : public Callable
{
public:
    Function(Interpreter *interpreter, FunctionDeclaration *decl);
    
    virtual std::string name() const noexcept override;

    virtual size_t arity() const noexcept override;

    virtual void invoke(const std::vector<ExpressionPtr>& args) override;

private:
    Interpreter *interpreter_;
    FunctionDeclaration *decl_;
};

}