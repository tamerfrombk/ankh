add_executable(lexer-tests lexer_tests.cc)
target_include_directories(lexer-tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(lexer-tests PRIVATE ankhlang Catch2::Catch2WithMain)
add_test(NAME lexer-tests COMMAND lexer-tests)

add_executable(parser-tests parser_tests.cc)
target_include_directories(parser-tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(parser-tests PRIVATE ankhlang Catch2::Catch2WithMain)
add_test(NAME parser-tests COMMAND parser-tests)

add_executable(interpreter-tests interpreter_tests.cc)
target_include_directories(interpreter-tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(interpreter-tests PRIVATE ankhlang Catch2::Catch2WithMain)
add_test(NAME interpreter-tests COMMAND interpreter-tests)

add_custom_target(all_tests DEPENDS run_all_tests)
add_custom_command(OUTPUT run_all_tests
  COMMAND lexer-tests
  COMMAND parser-tests
  COMMAND interpreter-tests
)