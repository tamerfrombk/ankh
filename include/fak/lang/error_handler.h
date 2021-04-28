#pragma once

#include <string>
#include <vector>

namespace fk::lang {
struct err_t 
{
    std::string msg;

    err_t(std::string msg)
        : msg(std::move(msg)) {}

};

class error_handler_t 
{
public:
    virtual ~error_handler_t() = default;

    virtual void report_error(const err_t& err) 
    {
        errors_.push_back(err);
    }

    virtual void report_error(err_t&& err)
    {
        errors_.push_back(err);
    }

    virtual size_t error_count() const noexcept
    {
        return errors_.size();
    }

    const std::vector<err_t>& errors() const noexcept
    {
        return errors_;
    }

private:
    std::vector<err_t> errors_;
};

}