#pragma once

#include <string>
#include <cstdio>
#include <cstdlib>
#include <string_view>

#include <fmt/core.h>
#include <fmt/format.h>

#include <ankh/def.h>
#include <ankh/pkg/path.h>

namespace ankh::log {

template <class... Args>
void debug(std::string_view file, const char *func, size_t line, const char *fmt, Args&&... args)
{
        const std::string_view path = ankh::pkg::parse_file_name_from_full_path(file);
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
ankh_NO_RETURN void fatal(std::string_view file, const char *func, size_t line, const char *fmt, Args&&... args)
{
        const std::string_view path = ankh::pkg::parse_file_name_from_full_path(file);
        auto str = fmt::format(fmt, path, func, line, std::forward<Args>(args)...);

        std::fputs(str.c_str(), stderr);

        std::exit(1);
}

}

#ifndef NDEBUG
        #define ankh_DEBUG(str, ...) ankh::log::debug(__FILE__, __FUNCTION__, __LINE__, "{}/{}() @ {}: "#str"\n",##__VA_ARGS__)
#else
        #define ankh_DEBUG(str, ...)
#endif

#define ankh_VERIFY(cond) do { if (!(cond)) { ankh::log::fatal(__FILE__, __FUNCTION__, __LINE__, "ASSERTION FAILURE @ {}/{}() @ {} since '( {} )' was false", #cond); }} while(0)

#define ankh_FATAL(str, ...) ankh::log::fatal(__FILE__, __FUNCTION__, __LINE__, "{}/{}() @ {}: "#str"\n",##__VA_ARGS__)