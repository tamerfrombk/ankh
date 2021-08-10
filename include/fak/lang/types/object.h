#pragma once

#include <memory>
#include <fak/lang/env.h>

namespace fk::lang {

template <class T>
struct Object
{
    EnvironmentPtr<T> env;

    Object(EnvironmentPtr<T> env)
        : env(env) {}
};

template <class T>
using ObjectPtr = std::shared_ptr<Object<T>>;

template <class T, class... Args>
ObjectPtr<T> make_object(Args&&... args) noexcept
{
    return std::make_shared<Object<T>>(std::forward<Args>(args)...);
}

}