#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

#include <fmt/core.h>
#include <fmt/format.h>

#include <ankh/def.hpp>

namespace ankh::log {

template <class... Args> void debug(const char *fmt, Args &&...args) {
    auto str = fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...);

    std::fputs(str.c_str(), stderr);
}

template <class... Args> void info(const char *fmt, Args &&...args) {
    std::fprintf(stdout, fmt, std::forward<Args>(args)...);
}

template <class... Args> void error(const char *fmt, Args &&...args) {
    std::fprintf(stderr, fmt, std::forward<Args>(args)...);
}

template <class... Args> ANKH_NO_RETURN void fatal(const char *fmt, Args &&...args) {
    auto str = fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...);

    std::fputs(str.c_str(), stderr);

    std::exit(1);
}

} // namespace ankh::log

#ifndef NDEBUG
#define ANKH_DEBUG(str, ...)                                                                                           \
    ankh::log::debug("{}/{}() @ {}: " #str "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define ANKH_DEBUG(str, ...)
#endif

#define ANKH_VERIFY(cond)                                                                                              \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            ankh::log::fatal(__FILE__, __FUNCTION__, __LINE__,                                                         \
                             "ASSERTION FAILURE @ {}/{}() @ {} since '( {} )' was false", #cond);                      \
        }                                                                                                              \
    } while (0)

#define ANKH_FATAL(str, ...)                                                                                           \
    ankh::log::fatal("{}/{}() @ {}: " #str "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
