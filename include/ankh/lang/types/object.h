#pragma once

#include <memory>
#include <string>

#include <ankh/lang/env.h>

namespace ankh::lang {

template <class T>
struct Object
{
    EnvironmentPtr<T> env;

    Object(EnvironmentPtr<T> env)
        : env(env) {}

    bool set(const std::string& name, T value) noexcept
    {
        return env->assign(name, value);
    }
};

template <class T>
using ObjectPtr = std::shared_ptr<Object<T>>;

template <class T, class... Args>
ObjectPtr<T> make_object(Args&&... args) noexcept
{
    return std::make_shared<Object<T>>(std::forward<Args>(args)...);
}

}