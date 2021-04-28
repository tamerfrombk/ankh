#include "fak/lang/expr.h"
#include <optional>
#include <utility>

#include <fak/lang/env.h>

bool fk::lang::environment::assign(const std::string &name, const expr_result &result) noexcept
{
    // TODO: implement a shadowing warning
    auto it = values_.insert({name, result});

    return it.second;
}

std::optional<fk::lang::expr_result> fk::lang::environment::value(const std::string& name) const noexcept
{
    const auto it = values_.find(name);

    return it == values_.end()
        ? std::nullopt
        : std::optional<expr_result>(it->second);
}
