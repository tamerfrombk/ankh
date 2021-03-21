#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLEAN_LOG_MAX_BUFFER_LENGTH (255)

#define CLEAN_LOG_IMPL(prefix, str, ...) do {\
        char debug_buf[CLEAN_LOG_MAX_BUFFER_LENGTH + 1] = {0};\
        strcpy(debug_buf, prefix);\
        strncat(debug_buf, str, CLEAN_LOG_MAX_BUFFER_LENGTH - strlen(prefix));\
        fprintf(stderr, debug_buf,##__VA_ARGS__);\
    } while (0)

#ifdef NDEBUG
    #define debug(str, ...)
#else
    #define debug(str, ...) CLEAN_LOG_IMPL("DEBUG: ", str,##__VA_ARGS__)
#endif

#define error(str, ...) CLEAN_LOG_IMPL("ERROR: ", str,##__VA_ARGS__)

#define fatal(str, ...) CLEAN_LOG_IMPL("FATAL: ", str,##__VA_ARGS__)
