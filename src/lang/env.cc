#include "clean/lang/expr.h"
#include <optional>
#include <utility>

#include <clean/lang/env.h>

bool environment_t::assign(const std::string &name, const expr_result_t &result) noexcept
{
    // TODO: implement a shadowing warning
    auto it = values_.insert({name, result});

    return it.second;
}

std::optional<expr_result_t> environment_t::value(const std::string& name) const noexcept
{
    const auto it = values_.find(name);

    return it == values_.end()
        ? std::nullopt
        : std::optional<expr_result_t>(it->second);
}
