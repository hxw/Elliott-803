# cpu library

set(src alu_test.c buffer_test.c core.c fpu_test.c processor.c reader.c alu.c convert.c cpu803.c fpu.c punch.c)

#add_library(803 SHARED ${src})
add_library(803 STATIC ${src})

target_include_directories(803 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(alu_test alu_test.c)
target_link_libraries(alu_test 803)

add_executable(buffer_test buffer_test.c)
target_link_libraries(buffer_test 803)

add_executable(fpu_test fpu_test.c)
target_link_libraries(fpu_test 803)
