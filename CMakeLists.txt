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

set(APP_EXE ankhsh)
set(APP_LIB ankh)
set(APP_SOURCES 
    src/ankh.cc 
    src/lang/lexer.cc 
    src/lang/token.cc 
    src/lang/parser.cc
    src/lang/expr.cc
    src/lang/interpreter.cc
    src/lang/static_analyzer.cc
    src/pkg/path.cc
)

set(FMT_LIB fmt)
set(FMT_SOURCES
    src/fmt/format.cc
    src/fmt/os.cc
)

add_library(${APP_LIB} STATIC ${APP_SOURCES})
add_library(${FMT_LIB} STATIC ${FMT_SOURCES})
add_executable(${APP_EXE} src/main.cc)

target_include_directories(${APP_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(${APP_LIB} PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(${FMT_LIB} PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(${APP_EXE} ${APP_LIB} ${FMT_LIB})

enable_testing()
add_subdirectory(test)
