#pragma once

#include <stdexcept>

#include <fmt/core.h>
#include <fmt/format.h>

#include <ankh/def.hpp>

#include <ankh/lang/token.hpp>

namespace ankh::lang {

struct ScanException : public std::runtime_error {
    explicit ScanException(const std::string &msg) : std::runtime_error(msg) {}
};

struct InterpretationException : public std::runtime_error {
    explicit InterpretationException(const std::string &msg) : std::runtime_error(msg) {}
};

struct ParseException : public std::runtime_error {
    explicit ParseException(const std::string &msg) : std::runtime_error(msg) {}
};

template <class E, class... Args> ANKH_NO_RETURN void panic(const Token &marker, const char *fmt, Args &&...args) {
    const std::string fmt_str = "{}:{}, " + std::string{fmt};
    const std::string str = fmt::format(fmt::runtime(fmt_str), marker.line, marker.col, std::forward<Args>(args)...);

    throw E(str);
}

template <class E, class... Args> ANKH_NO_RETURN void builtin_panic(const char *name, const char *fmt, Args &&...args) {
    const std::string fmt_str = "{}:{}, " + std::string{fmt};
    const std::string str = fmt::format(fmt::runtime(fmt_str), "BUILTIN", name, std::forward<Args>(args)...);

    throw E(str);
}

} // namespace ankh::lang
