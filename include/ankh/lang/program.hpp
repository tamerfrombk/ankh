#pragma once

#include <vector>
#include <string>

#include <ankh/lang/statement.hpp>
#include <ankh/lang/hop_table.hpp>


namespace ankh::lang {

struct Program
{
    std::vector<StatementPtr> statements;
    std::vector<std::string> errors;
    HopTable hop_table;

    bool has_errors() const noexcept
    {
        return errors.size() > 0;
    }

    std::size_t size() const noexcept
    {
        return statements.size();
    }

    const StatementPtr& operator[](size_t i) const noexcept
    {
        return statements[i];
    }
};

}