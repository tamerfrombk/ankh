#pragma once

#include <string>
#include <unordered_map>
#include <optional>

#include <fak/lang/expr.h>

namespace fk::lang {
class environment
{
public:
    bool assign(const std::string& name, const expr_result& result) noexcept;

    std::optional<expr_result> value(const std::string& name) const noexcept;

private:
    std::unordered_map<std::string, expr_result> values_;
};

}