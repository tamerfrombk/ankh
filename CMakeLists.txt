CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)
PROJECT(fak VERSION 1.0.0)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# @PLATFORM: this works with Clang/GCC but will be an issue with MSVC
SET(CMAKE_CXX_WARNING_FLAGS "-Wall -Wextra -Wpedantic")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_WARNING_FLAGS}")

SET(APP_EXE fak)
SET(APP_LIB fakapi)

SET(SOURCES 
    src/fak.cc 
    src/lang/lexer.cc 
    src/lang/token.cc 
    src/lang/parser.cc
    src/lang/expr.cc
    src/lang/interpreter.cc
    src/lang/env.cc
)

ADD_LIBRARY(${APP_LIB} STATIC ${SOURCES})
ADD_EXECUTABLE(${APP_EXE} src/main.cc)

TARGET_INCLUDE_DIRECTORIES(${APP_EXE} PRIVATE ${CMAKE_SOURCE_DIR}/include)
TARGET_INCLUDE_DIRECTORIES(${APP_LIB} PRIVATE ${CMAKE_SOURCE_DIR}/include)
TARGET_LINK_LIBRARIES(${APP_EXE} readline ${APP_LIB})

# tests
ADD_SUBDIRECTORY(test)
