#include <utility>

#include <fak/lang/env.h>

bool fk::lang::environment::assign(const std::string &name, const expr_result &result) noexcept
{
    // TODO: implement a shadowing warning
    values_[name] = result;

    return true;
}

std::optional<fk::lang::expr_result> fk::lang::environment::value(const std::string& name) const noexcept
{
    const auto it = values_.find(name);

    return it == values_.end()
        ? std::nullopt
        : std::optional<expr_result>(it->second);
}

bool fk::lang::environment::contains(const std::string &key) const noexcept
{ 
    return values_.count(key) > 0; 
}
