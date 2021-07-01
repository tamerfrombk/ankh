#pragma once

#include <vector>
#include <string>

#include <fak/lang/statement.h>

namespace fk::lang {

class Program
{
public:
    void add_statement(StatementPtr stmt) noexcept
    {
        statements_.push_back(std::move(stmt));
    }

    void add_error(std::string&& error) noexcept
    {
        errors_.push_back(std::forward<std::string>(error));
    }

    bool has_errors() const noexcept
    {
        return errors_.size() > 0;
    }

    const std::vector<std::string>& errors() const noexcept
    {
        return errors_;
    }

    const std::vector<StatementPtr>& statements() const noexcept
    {
        return statements_;
    }

    std::size_t size() const noexcept
    {
        return statements_.size();
    }

    const StatementPtr& operator[](size_t i) const noexcept
    {
        return statements_[i];
    }

private:
    std::vector<StatementPtr> statements_;
    std::vector<std::string> errors_;
};

}