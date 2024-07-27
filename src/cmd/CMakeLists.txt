add_executable(ankhsh ankhsh.cc)
target_include_directories(ankhsh PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ankhsh PRIVATE ankhlang)