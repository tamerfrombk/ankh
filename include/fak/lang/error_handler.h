#pragma once

#include <string>
#include <vector>

namespace fk::lang {
struct error 
{
    std::string msg;

    error(std::string msg)
        : msg(std::move(msg)) {}

};

class error_handler 
{
public:
    virtual ~error_handler() = default;

    virtual void report_error(const error& err) 
    {
        errors_.push_back(err);
    }

    virtual void report_error(error&& err)
    {
        errors_.push_back(err);
    }

    virtual size_t error_count() const noexcept
    {
        return errors_.size();
    }

    const std::vector<error>& errors() const noexcept
    {
        return errors_;
    }

private:
    std::vector<error> errors_;
};

}