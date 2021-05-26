#pragma once

#include <string>
#include <unordered_map>
#include <optional>

#include <fak/lang/expr.h>

namespace fk::lang {
class environment
{
public:
    bool assign(const std::string& name, const ExprResult& result) noexcept;

    std::optional<ExprResult> value(const std::string& name) const noexcept;

    bool contains(const std::string& key) const noexcept;

private:
    std::unordered_map<std::string, ExprResult> values_;
};

}