cmake_minimum_required(VERSION 3.9)
project(stockfish CXX)

set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

set (CMAKE_CXX_FLAGS "-O3 -msse3 -mpopcnt -DNDEBUG")
set (CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed")
set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
set (THREADS_PREFER_PTHREAD_FLAG TRUE)
set (CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
find_package(Threads REQUIRED)

file(GLOB SRCS src/*.cpp src/syzygy/*.cpp src/*.h src/syzygy/*.h)
add_executable(stockfish ${SRCS})
target_link_libraries(stockfish Threads::Threads)
