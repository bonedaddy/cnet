#####################
# START CMAKE SETUP #
#####################

cmake_minimum_required(VERSION 3.0)
project(libcp2p LANGUAGES C)
set(CMAKE_C_COMPILER /usr/bin/gcc)


# handle debug build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # -g indicates to include debug symbols
    list(APPEND flags -std=c17 -Wall -Wextra -Werror -pedantic -g)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Analyze")
    # override gcc compiler to v10 for static analysis
    set(CMAKE_C_COMPILER /usr/bin/gcc-10)
    list(APPEND flags -std=c17 -Wall -Wextra -Werror -pedantic -g -fanalyzer)
    message(STATUS "enabling static analysis")
endif()

# handle ci build type
if(CMAKE_BUILD_TYPE STREQUAL "CI")
    set(CMAKE_BUILD_TYPE "Release")
    # override the previous C compiler choice
    set(CMAKE_C_COMPILER /usr/bin/gcc-9)
endif()


# handle no build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    list(APPEND flags -std=c17 -Wall -Wextra -Werror -pedantic)
endif()

list(APPEND flags -D_POSIX_C_SOURCE=201112L)


file(GLOB_RECURSE ULOG_SOURCES
    ./deps/ulog/*.h
    ./deps/ulog/*.c
)

add_library(ulog ${ULOG_SOURCES})
target_compile_options(ulog PRIVATE ${flags})
target_link_libraries(ulog pthread)

add_library(libsockets ./sockets.c ./sockets.h)
target_compile_options(libsockets PRIVATE ${flags})
target_link_libraries(libsockets pthread ulog)


add_executable(cli ./main.c)
target_link_libraries(cli pthread)
target_compile_options(cli PRIVATE ${flags})
