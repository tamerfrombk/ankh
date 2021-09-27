#pragma once

#include <utility>
#include <memory>


namespace ankh::lang {

template <class T>
class SharedValue
{
public:
    SharedValue()          : value_(std::make_shared<T>(T{})) {}
    SharedValue(T&& e)     : value_(std::make_shared<T>(std::forward<T>(e))) {}
    SharedValue(const T& e): value_(std::make_shared<T>(e)) {}

    friend bool operator==(const SharedValue<T>& lhs, const SharedValue<T>& rhs) noexcept
    {
        return *lhs.value_ == *rhs.value_;
    }

    friend bool operator!=(const SharedValue<T>& lhs, const SharedValue<T>& rhs) noexcept
    {
        return !(operator==(lhs, rhs));
    }

    friend T& operator*(SharedValue<T>& obj) {
        return *obj.value_;
    }

    friend const T& operator*(const SharedValue<T>& obj) {
        return *obj.value_;
    }

    T* operator->() {
        return value_.get();
    }

    const T* operator->() const {
        return value_.get();
    }

private:
    std::shared_ptr<T> value_;
};

}