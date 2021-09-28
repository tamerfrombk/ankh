#pragma once

#ifdef __linux__
    #include <ankh/sys/linux.h>
#else
    #error "this operating system isn't supported"
#endif