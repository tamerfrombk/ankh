#pragma once

#include <memory>
#include <string>
#include <vector>

#include <fak/lang/env.h>
#include <fak/lang/callable.h>

namespace fk::lang {

// TODO: fix all the callables to take the interpreter in the invoke() function
// and templatize that
template <class T, class I>
class Data
    : public Callable
{
public:
    Data(I *interpreter, std::string name, EnvironmentPtr<T> env, std::vector<std::string> members)
        : interpreter_(interpreter), name_(std::move(name)), env_(env), members_(std::move(members)) {}

    virtual std::string name() const noexcept override
    {
        return name_;
    }

    virtual size_t arity() const noexcept override
    {
        return members_.size();
    }

    virtual void invoke(const std::vector<ExpressionPtr>& args) override
    {
        for (size_t i = 0; i < args.size(); ++i) {
            const ExprResult result = interpreter_->evaluate(args[i]);
            if (!env_->assign(members_[i], result)) {
                FK_FATAL("unable to declare {} in {} data constructor", members_[i], name());
            }
        }
    }

    EnvironmentPtr<T> env() noexcept
    {
        return env_;
    }

private:
    I* interpreter_;
    std::string name_;
    EnvironmentPtr<T> env_;
    std::vector<std::string> members_;
};

}