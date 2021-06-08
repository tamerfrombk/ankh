#include <utility>

#include <fak/lang/env.h>


fk::lang::Environment::Environment(fk::lang::Environment *enclosing)
    : enclosing_(enclosing)
    , scope_(enclosing_ == nullptr ? 0 : 1 + enclosing->scope()) 
{}

bool fk::lang::Environment::assign(const std::string& name, const ExprResult& result) noexcept
{
    // TODO: this contains check bothers me and forces me to implement put()
    // Figure out if this is strictly necessary
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

void fk::lang::Environment::put(const std::string& name, const ExprResult& result) noexcept
{
    FK_DEBUG("PUT '{} = '{}' @ scope '{}'", name, result.stringify(), scope());

    values_[name] = result;
}

std::optional<fk::lang::ExprResult> fk::lang::Environment::value(const std::string& name) const noexcept
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

bool fk::lang::Environment::contains(const std::string& key) const noexcept
{ 
    return values_.count(key) > 0; 
}

size_t fk::lang::Environment::scope() const noexcept
{
    return scope_;
}
