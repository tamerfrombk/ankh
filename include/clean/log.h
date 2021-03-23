#pragma once

#include <stdio.h>
#include <stdlib.h>

// These macros where created with the help of this answer from Stack Overflow
// https://stackoverflow.com/a/1644898

#ifdef NDEBUG
    #define debug(str, ...)
#else
    #define debug(fmt, ...) \
            do { fprintf(stderr, "DEBUG: [ %s:%d | %s() ]: " fmt, __FILE__, __LINE__, __func__,##__VA_ARGS__); } while (0)
#endif

#define info(fmt, ...) \
        do { fprintf(stdout, "INFO: " fmt,##__VA_ARGS__); } while (0)

#define error(fmt, ...) \
        do { fprintf(stderr, "ERROR: " fmt,##__VA_ARGS__); } while (0)

#define fatal(fmt, ...) \
        do { fprintf(stderr, "FATAL: " fmt,##__VA_ARGS__); exit(EXIT_FAILURE); } while (0)

#define ASSERT_NOT_NULL(ptr) \
    if ((ptr) == NULL) { fprintf(stderr, "NULLPTR @ [ %s:%d | %s() ]", __FILE__, __LINE__, __func__); exit(EXIT_FAILURE); }
