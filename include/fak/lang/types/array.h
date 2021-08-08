#pragma once

#include <vector>
#include <memory>

namespace fk::lang {

template <class T>
class Array
{
    using ArrayType = std::vector<T>;

public:
    Array()                : elems_(std::make_shared<ArrayType>()) {}
    Array(ArrayType elems) : elems_(std::make_shared<ArrayType>(std::move(elems))) {}

    void append(const T& elem) noexcept
    {
        elems_->push_back(elem);
    }

    bool empty() const noexcept
    {
        return elems_->empty();
    }

    T& operator[](size_t i) noexcept
    {
        return (*elems_)[i];
    }

    const T& operator[](size_t i) const noexcept
    {
        return (*elems_)[i];
    }

    size_t size() const noexcept
    {
        return elems_->size();
    }

    friend bool operator==(const Array<T>& lhs, const Array<T>& rhs) noexcept
    {
        return *lhs.elems_ == *rhs.elems_;
    }

    friend bool operator!=(const Array<T>& lhs, const Array<T>& rhs) noexcept
    {
        return !(operator==(lhs, rhs));
    }

private:
    std::shared_ptr<ArrayType> elems_;
};

}