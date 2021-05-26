#include <utility>

#include <fak/lang/env.h>

bool fk::lang::Environment::assign(const std::string &name, const ExprResult &result) noexcept
{
    // TODO: implement a shadowing warning
    values_[name] = result;

    return true;
}

std::optional<fk::lang::ExprResult> fk::lang::Environment::value(const std::string& name) const noexcept
{
    const auto it = values_.find(name);

    return it == values_.end()
        ? std::nullopt
        : std::optional<ExprResult>(it->second);
}

bool fk::lang::Environment::contains(const std::string &key) const noexcept
{ 
    return values_.count(key) > 0; 
}
