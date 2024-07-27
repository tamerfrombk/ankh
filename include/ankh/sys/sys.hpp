#pragma once

#ifdef __linux__
    #include <ankh/sys/linux.hpp>
#else
    #error "this operating system isn't supported"
#endif