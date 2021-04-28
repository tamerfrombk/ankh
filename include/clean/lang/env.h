#pragma once

#include <string>
#include <unordered_map>
#include <optional>

#include <clean/lang/expr.h>

class environment_t
{
public:
    bool assign(const std::string& name, const expr_result_t& result) noexcept;
    
    std::optional<expr_result_t> value(const std::string& name) const noexcept;

private:
    std::unordered_map<std::string, expr_result_t> values_;
};