#pragma once

#include <string>
#include <cstdio>
#include <cstdlib>

#include <fak/def.h>


namespace fk::log {

template <class... Args>
void debug(const char *fmt, Args&&... args)
{
#ifndef NDEBUG
        std::fprintf(stderr, fmt, std::forward<Args>(args)...);
#endif
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
FK_NO_RETURN void fatal(const char *fmt, Args&&... args)
{
        std::fprintf(stderr, fmt, std::forward<Args>(args)...);
        std::exit(1);
}

}
