#pragma once

#include <string>
#include <cstdio>
#include <cstdlib>
#include <string_view>

#include <fmt/core.h>
#include <fmt/format.h>

#include <fak/def.h>
#include <fak/pkg/path.h>

namespace fk::log {

template <class... Args>
void debug(std::string_view file, const char *func, size_t line, const char *fmt, Args&&... args)
{
        const std::string_view path = fk::pkg::parse_file_name_from_full_path(file);
        auto str = fmt::format(fmt, path, func, line, std::forward<Args>(args)...);

        std::fputs(str.c_str(), stderr);
}

template <class... Args>
void info(const char *fmt, Args&&... args)
{
        std::fprintf(stdout, fmt, std::forward<Args>(args)...);
}

template <class... Args>
void error(const char *fmt, Args&&... args)
{
        std::fprintf(stderr, fmt, std::forward<Args>(args)...);
}

template <class... Args>
FK_NO_RETURN void fatal(std::string_view file, const char *func, size_t line, const char *fmt, Args&&... args)
{
        const std::string_view path = fk::pkg::parse_file_name_from_full_path(file);
        auto str = fmt::format(fmt, path, func, line, std::forward<Args>(args)...);

        std::fputs(str.c_str(), stderr);

        std::exit(1);
}

}

#ifndef NDEBUG
        #define FK_DEBUG(str, ...) fk::log::debug(__FILE__, __FUNCTION__, __LINE__, "{}/{}() @ {}: "#str"\n",##__VA_ARGS__)
#else
        #define FK_DEBUG(str, ...)
#endif

#define FK_VERIFY(cond) do { if (!(cond)) { fk::log::fatal(__FILE__, __FUNCTION__, __LINE__, "ASSERTION FAILURE @ {}/{}() @ {} since '( {} )' was false", #cond); }} while(0)

#define FK_FATAL(str, ...) fk::log::fatal(__FILE__, __FUNCTION__, __LINE__, "{}/{}() @ {}: "#str"\n",##__VA_ARGS__)