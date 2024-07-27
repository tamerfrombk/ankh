add_library(ankhlang STATIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/../ankh.cc 
    lexer.cc 
    token.cc 
    parser.cc
    expr.cc
    interpreter.cc
    static_analyzer.cc
)

target_link_libraries(ankhlang PRIVATE fmt::fmt)
target_include_directories(ankhlang PRIVATE ${CMAKE_SOURCE_DIR}/include)