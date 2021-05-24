#pragma once

#include <string>
#include <vector>

namespace fk::lang {
struct Error 
{
    std::string msg;

    Error(std::string msg)
        : msg(std::move(msg)) {}

};

class ErrorHandler 
{
public:
    virtual ~ErrorHandler() = default;

    virtual void report_error(const Error& err) 
    {
        errors_.push_back(err);
    }

    virtual void report_error(Error&& err)
    {
        errors_.push_back(err);
    }

    virtual size_t error_count() const noexcept
    {
        return errors_.size();
    }

    const std::vector<Error>& errors() const noexcept
    {
        return errors_;
    }

private:
    std::vector<Error> errors_;
};

}