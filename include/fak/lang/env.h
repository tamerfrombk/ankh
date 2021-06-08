#pragma once

#include <string>
#include <unordered_map>
#include <optional>
#include <memory>
#include <utility>

#include <fak/lang/expr.h>

#include <fak/log.h>

namespace fk::lang {
class Environment
{
public:
    Environment(Environment *enclosing = nullptr);

    FK_NO_DISCARD bool assign(const std::string& name, const ExprResult& result) noexcept;

    void put(const std::string& name, const ExprResult& result) noexcept;

    std::optional<ExprResult> value(const std::string& name) const noexcept;

    bool contains(const std::string& key) const noexcept;

    size_t scope() const noexcept;

private:
    std::unordered_map<std::string, ExprResult> values_;
    Environment *enclosing_;
    const size_t scope_;
};

using EnvironmentPtr = std::shared_ptr<Environment>;

template <class... Args>
EnvironmentPtr make_env(Args&&... args) noexcept
{
    return std::make_shared<Environment>(std::forward<Args>(args)...);
}

}