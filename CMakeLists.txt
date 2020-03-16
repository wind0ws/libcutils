cmake_minimum_required(VERSION 3.1)
project(MIXED_PLATFORM VERSION 0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
#set(CMAKE_C_EXTENSIONS FALSE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc/)

#add_library(mplite ../../src/mplite.c)
#target_include_directories(mplite PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc/)

add_executable(test
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xlog.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/thread_wrapper.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/semaphore_windows.c
        ${CMAKE_CURRENT_SOURCE_DIR}/test/test.c)



