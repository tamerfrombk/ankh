cmake_minimum_required(VERSION 3.25.0)
project(ankh LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

if (WIN32)
    add_compile_options(/W3)
else() 
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_subdirectory(vendor)
add_subdirectory(src)

enable_testing()
add_subdirectory(test)
