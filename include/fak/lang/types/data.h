#pragma once

#include <memory>
#include <string>
#include <vector>

#include <fak/lang/env.h>

namespace fk::lang {

template <class T>
struct Data
{
    std::string name;
    EnvironmentPtr<T> env;
    std::vector<std::string> members;
    
    Data(std::string name, EnvironmentPtr<T> env, std::vector<std::string> members)
        : name(std::move(name)), env(env), members(std::move(members)) {}
};

template <class T>
using DataPtr = std::unique_ptr<Data<T>>;

template <class T, class... Args>
DataPtr<T> make_data(Args&&... args) noexcept
{
    return std::make_unique<Data<T>>(std::forward<Args>(args)...);
}

}