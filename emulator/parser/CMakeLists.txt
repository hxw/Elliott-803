# parser library

#add_library(parser SHARED parser.c)

add_library(parser STATIC parser.c)

target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(parser_test parser_test.c)
target_link_libraries(parser_test parser)
