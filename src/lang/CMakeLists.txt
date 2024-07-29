add_library(ankhlang STATIC 
    lexer.cc 
    token.cc 
    parser.cc
    expr.cc
    interpreter.cc
    static_analyzer.cc
)

target_include_directories(ankhlang PRIVATE ${CMAKE_SOURCE_DIR}/include)