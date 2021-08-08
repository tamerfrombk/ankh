#pragma once

#include <memory>
#include <string>

#include <fak/lang/env.h>

namespace fk::lang {

class Data
{
public:
    Data(std::string name, EnvironmentPtr env)
        : name_(std::move(name)), env_(env) {}
    
private:
    std::string name_;
    EnvironmentPtr env_;
};

using DataPtr = std::unique_ptr<Data>;

template <class... Args>
DataPtr make_data(Args&&... args) noexcept
{
    return std::make_unique<Data>(std::forward<Args>(args)...);
}

}