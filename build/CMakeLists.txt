cmake_minimum_required(VERSION 3.1)



project(libcutils VERSION 0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
#set(CMAKE_C_EXTENSIONS FALSE)

set(CMAKE_VERBOSE_MAKEFILE ON)

#扫描指定scan_dir 目录及其子目录下的 .h 文件所在目录，存放到 return_list 中
MACRO(SCAN_HEADER_DIRECTORIES scan_dir return_list)
    FILE(GLOB_RECURSE new_list ${scan_dir}/*.h)
    SET(dir_list "")
    FOREACH (file_path ${new_list})
        GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
        SET(dir_list ${dir_list} ${dir_path})
    ENDFOREACH ()
    LIST(REMOVE_DUPLICATES dir_list)
    SET(${return_list} ${dir_list})
ENDMACRO()

get_filename_component(SRC_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)
message(STATUS "\n CMAKE_CURRENT_SOURCE_DIR => ${CMAKE_CURRENT_SOURCE_DIR}\n SRC_DIR=${SRC_DIR}\n")

#add_definitions(-D_CRT_SECURE_NO_WARNINGS)

include_directories(${SRC_DIR}/inc/)

#add_library(mplite ../../src/mplite.c)
#target_include_directories(mplite PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc/)

add_executable(libcutils_test
		${SRC_DIR}/src/time_util.c
		${SRC_DIR}/src/file_util.c
		${SRC_DIR}/src/msg_queue_handler.c
		${SRC_DIR}/src/ring_msg_queue.c
		${SRC_DIR}/src/ringbuffer.c
		${SRC_DIR}/src/file_logger.c
        ${SRC_DIR}/src/xlog.c
		${SRC_DIR}/src/ring_buf.c
		${SRC_DIR}/src/pthread_windows.c
        ${SRC_DIR}/src/thread_wrapper.c
        ${SRC_DIR}/src/semaphore_windows.c
		${SRC_DIR}/src/strings.c
		${SRC_DIR}/test/file_util_test.c
		${SRC_DIR}/test/file_logger_test.c
		${SRC_DIR}/test/main.c)

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

#find_library( # Sets the name of the path variable.
#        z-lib
#
#        # Specifies the name of the NDK library that
#        # you want CMake to locate.
#        z)
		

target_link_libraries( # Specifies the target library.
        libcutils_test

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib}
        )		

