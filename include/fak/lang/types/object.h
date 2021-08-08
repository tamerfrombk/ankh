#pragma once

#include <memory>

namespace fk::lang {

class Data;

class Object
{
public:
    Object(Data *data)
        : data_(data) {}

private:
    Data *data_;
};

using ObjectPtr = std::shared_ptr<Object>;

template <class... Args>
ObjectPtr make_object(Args&&... args) noexcept
{
    return std::make_shared<Object>(std::forward<Args>(args)...);
}

}