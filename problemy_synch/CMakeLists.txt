message("----------------------------------------")
message("PROGRAMOWANIE WSPOLBIEZNE CDV")
message("----------------------------------------")

cmake_minimum_required(VERSION 3.22)
project(prog_wspol)

# -------------------------------------------------------------------------------- #

set(CMAKE_C_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c17 -pthread -lrt  -s -O2 -Wall -pedantic -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread -lrt  -s -O2 -Wall -pedantic -Werror")

# -------------------------------------------------------------------------------- #

set(TRG_SANTA santa_claus_problem)

set(SRC_SANTA
        santa_claus_mutex.c
        barrier.c
        )

# -------------------------------------------------------------------------------- #

set(TRG_WATER water_synthesis_problem)

set(SRC_WATER
        synteza_wody_sem.c
        )

# -------------------------------------------------------------------------------- #

message("BUILDING FOR LINUX (POSIX)")

add_definitions(-D_POSIX_C_SOURCE=199309L)

add_executable(${TRG_SANTA} ${SRC_SANTA} ${SRCS})
add_executable(${TRG_WATER} ${SRC_WATER} ${SRCS})

# -------------------------------------------------------------------------------- #

message("----------------------------------------")

message(Project:${PROJECT_NAME})
message("CMAKE_FLAGS:${CMAKE_C_FLAGS}")

message("----------------------------------------")
