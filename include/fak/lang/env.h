#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <memory>
#include <utility>

#include <fak/log.h>

namespace fk::lang {

template <class T>
class Environment;

template <class T>
using EnvironmentPtr = std::shared_ptr<Environment<T>>;

template <class T>
class Environment
{
public:
    Environment(EnvironmentPtr<T> enclosing = nullptr)
        : enclosing_(enclosing)
        , scope_(enclosing_ == nullptr ? 0 : 1 + enclosing->scope()) {}

    FK_NO_DISCARD bool assign(const std::string& name, const T& result) noexcept
    {
        if (contains(name)) {
            FK_DEBUG("ASSIGNMENT '{}' = '{}' @ scope '{}'", name, result.stringify(), scope());

            values_[name] = result;

            return true;
        }

        if (enclosing_ != nullptr) {
            FK_DEBUG("ASSIGNMENT LOOKUP '{}' = '{}' @ enclosing scope '{}'", name, result.stringify(), enclosing_->scope());
            return enclosing_->assign(name, result);
        }

        return false;
    }

    FK_NO_DISCARD bool declare(const std::string& name, const T& result) noexcept
    {
        FK_DEBUG("PUT '{}' = '{}' @ scope '{}'", name, result.stringify(), scope());
        if (contains(name)) {
            FK_DEBUG("'{}' cannot be declared because it already exists in scope {}", name, scope());
            return false;
        }

        values_[name] = result;

        return true;
    }

    std::optional<T> value(const std::string& name) const noexcept
    {
        if (const auto it = values_.find(name); it != values_.end()) {
            FK_DEBUG("IDENTIFIER '{}' = '{}' @ scope '{}'", name, it->second.stringify(), scope());
            return { it->second };
        }

        if (enclosing_ != nullptr) {
            FK_DEBUG("IDENTIFIER LOOKUP '{}' @ enclosing scope '{}'", name, enclosing_->scope());
            return enclosing_->value(name);
        }

        return std::nullopt;
    }

    bool contains(const std::string& key) const noexcept
    {
        return values_.count(key) > 0;
    }

    size_t scope() const noexcept
    {
        return scope_;
    }

private:
    std::unordered_map<std::string, T> values_;
    EnvironmentPtr<T> enclosing_;
    const size_t scope_;
};

template <class T, class... Args>
EnvironmentPtr<T> make_env(Args&&... args) noexcept
{
    return std::make_shared<Environment<T>>(std::forward<Args>(args)...);
}

}