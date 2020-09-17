cmake_minimum_required(VERSION 3.18)
project(emu803)

set(src main.c emulator.c commands.c pathsearch.c)

option(STRICT "strict compilation flags" FALSE)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -pedantic -Werror -std=c17")

if(STRICT)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Weverything -Wno-padded -Wno-switch-enum -Wno-unused-parameter")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-sign-conversion -Wno-vla -Wno-pedantic -Wno-shift-sign-overflow -Wno-format-nonliteral")
endif()

# cpp definitions
add_definitions(-DPROGRAM_STRING="emu803")
if(DEFINED VERSION)
  add_definitions(-DVERSION_STRING="${VERSION}")
else()
  add_definitions(-DVERSION_STRING="zero")
endif()
if(DEFINED DEFAULT_TAPE_DIR)
  add_definitions(-DDEFAULT_TAPE_DIR="${DEFAULT_TAPE_DIR}")
endif()

# local sub directories
add_subdirectory(cpu)
add_subdirectory(io5)
add_subdirectory(parser)

#not sure why this line is needed
set(CURSES_INCLUDE_PATH /usr/lib /usr/local/lib)

# locade wide char curses
set(CURSES_NEED_WIDE TRUE)
#set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CURSES_CFLAGS}")

# use pthreads
find_package(Threads REQUIRED)


#set(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -lcursesw -lthr -lrt -Lcpu -l803 -Lio5 -lio5 -Lparser -lparser")

add_executable(emu803 ${src})
target_link_libraries(emu803 LINK_PUBLIC 803 io5 parser ${CURSES_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
