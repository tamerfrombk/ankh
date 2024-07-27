Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0
)

FetchContent_MakeAvailable(Catch2)

add_executable(lexer-tests lexer_tests.cc)
target_include_directories(lexer-tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(lexer-tests PRIVATE ${TEST_LIB} ${APP_LIB} ${FMT_LIB} Catch2::Catch2WithMain)
add_test(lexer-tests COMMAND lexer-tests)

add_executable(parser-tests parser_tests.cc)
target_include_directories(parser-tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(parser-tests PRIVATE ${TEST_LIB} ${APP_LIB} ${FMT_LIB} Catch2::Catch2WithMain)
add_test(parser-tests COMMAND parser-tests)

add_executable(interpreter-tests interpreter_tests.cc)
target_include_directories(interpreter-tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(interpreter-tests PRIVATE ${TEST_LIB} ${APP_LIB} ${FMT_LIB} Catch2::Catch2WithMain)
add_test(interpreter-tests COMMAND interpreter-tests)

add_custom_target(all_tests 
  COMMAND lexer-tests
  COMMAND parser-tests
  COMMAND interpreter-tests
)