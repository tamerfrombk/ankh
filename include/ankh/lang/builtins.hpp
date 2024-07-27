#pragma once

#include <ankh/log.hpp>
#include <ankh/lang/callable.hpp>

#define ANKH_DECLARE_BUILTIN_TYPE(ClassName, method) \
template <class T, class I>\
class ClassName\
    : public BuiltIn<T, I>\
{\
public:\
    ClassName(I* interpreter, EnvironmentPtr<T> closure, std::string name, int arity)\
        : BuiltIn<T,I>(interpreter, closure, name, arity) {}\
\
protected:\
    virtual void invoke(const std::vector<ExprResult>& args) override\
    {\
        this->interpreter_->method(args);\
    }\
}

namespace ankh::lang {

template <class T, class I>
class BuiltIn
    : public Callable
{
public:
    BuiltIn(I* interpreter, EnvironmentPtr<T> closure, std::string name, size_t arity)
        : interpreter_(interpreter)
        , closure_(closure)
        , name_(name)
        , arity_(arity) {}
    
    virtual std::string name() const noexcept override
    {
        return name_;
    }

    virtual size_t arity() const noexcept override
    {
        return arity_;
    }

    virtual void invoke(const std::vector<ExpressionPtr>& args) override
    {
        EnvironmentPtr<T> environment(make_env<T>(closure_));
        ANKH_DEBUG("closure environment {} created", environment->scope());

        for (size_t i = 0; i < arity(); ++i) {
            ANKH_VERIFY(environment->declare("param" + std::to_string(i), {}));
        }

        std::vector<ExprResult> evaluated_args;
        for (size_t i = 0; i < args.size(); ++i) {
            evaluated_args.push_back(interpreter_->evaluate(args[i]));
        }

        invoke(evaluated_args);
    }
protected:
    virtual void invoke(const std::vector<ExprResult>& args) = 0;

protected:
    I *interpreter_;

private:
    EnvironmentPtr<T> closure_;
    const std::string name_;
    const size_t arity_;
};

// System Builtins
ANKH_DECLARE_BUILTIN_TYPE(PrintFn, print);
ANKH_DECLARE_BUILTIN_TYPE(ExitFn, exit);
ANKH_DECLARE_BUILTIN_TYPE(LengthFn, length);
ANKH_DECLARE_BUILTIN_TYPE(ExportFn, exportfn);

// Number Builtins
ANKH_DECLARE_BUILTIN_TYPE(IntFn, cast_int);

// Array Builtins
ANKH_DECLARE_BUILTIN_TYPE(AppendFn, append);

// String Builtins
ANKH_DECLARE_BUILTIN_TYPE(StrFn, str);

// Dictionary Builtins
ANKH_DECLARE_BUILTIN_TYPE(KeysFn, keys);
}