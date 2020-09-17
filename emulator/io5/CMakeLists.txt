# cpu library

set(src allocation.c get.c put.c read.c write.c conv_test.c open.c read_test.c write_test.c)

#add_library(io5 SHARED)
add_library(io5 STATIC ${src})

target_include_directories(io5 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(conv_test conv_test.c)
target_link_libraries(conv_test io5)

add_executable(read_test read_test.c)
target_link_libraries(read_test io5)

add_executable(write_test write_test.c)
target_link_libraries(write_test io5)
