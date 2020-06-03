cmake_minimum_required(VERSION 3.1)

if ( WIN32 AND NOT CYGWIN AND NOT ( CMAKE_SYSTEM_NAME STREQUAL "WindowsStore" ) AND NOT ANDROID)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT" CACHE STRING "")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd" CACHE STRING "")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT" CACHE STRING "")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd" CACHE STRING "")
endif ()

project(libcutils VERSION 1.0 LANGUAGES C CXX)

#set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD 11)
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

message(STATUS "\nCurrent CMakeLists.txt => ${CMAKE_CURRENT_LIST_DIR}\n")
#get lcu root dirs,because CMakeLists.txt is in "build" dir, not in root dir. we should back to parent folder.
get_filename_component(LCU_ROOT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)
message(STATUS "\n CMAKE_CURRENT_SOURCE_DIR => ${CMAKE_CURRENT_SOURCE_DIR}\n LCU_ROOT_DIR=${LCU_ROOT_DIR}\n")

#add_definitions(-D_CRT_SECURE_NO_WARNINGS)
#For compiler
#add_compile_options("-DCURL_STATICLIB")

#add compile/optimize options for DEBUG/RELEASE
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS_RELEASE} -Wall" CACHE STRING "")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS_RELEASE} -Wall -std=c++11 -fexceptions" CACHE STRING "")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2" CACHE STRING "")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Og" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2" CACHE STRING "")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og" CACHE STRING "")

if(WIN32)
#supress "C4477" for VS
add_definitions( /wd4477 )
else()
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
# Android 5.0 以上需要在此处设置 PIE
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE -pie")
endif(WIN32)

SCAN_HEADER_DIRECTORIES(${LCU_ROOT_DIR}/inc/ LCU_HEADERS)
message(STATUS "LCU_HEADERS => \n ${LCU_HEADERS}")
include_directories(${LCU_HEADERS})
#include_directories(${LCU_ROOT_DIR}/inc/)
#for publish library: copy header to output dir
file(COPY ${LCU_ROOT_DIR}/inc/  DESTINATION  ${CMAKE_CURRENT_LIST_DIR}/output/include/)

#add_library(mplite ../../src/mplite.c)
#target_include_directories(mplite PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc/)

#collect src/*.c to "LCU_SRCS" 
aux_source_directory(${LCU_ROOT_DIR}/src LCU_SRCS)
message(STATUS "\n LCU_SRCS => ${LCU_SRCS}\n ")

add_library(lcu_static
			STATIC
			${LCU_SRCS})
add_library(lcu_shared
			SHARED
			${LCU_SRCS})
# 指定库的输出名称
set_target_properties(lcu_static PROPERTIES OUTPUT_NAME "lcu")
set_target_properties(lcu_shared PROPERTIES OUTPUT_NAME "lcu")
# 使动态库和静态库同时存在
set_target_properties(lcu_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(lcu_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
# 指定动态库版本
# VERSION 动态库版本
# SOVERSION API版本
set_target_properties(lcu_shared PROPERTIES VERSION 1.0 SOVERSION 1)

#set(LCU_TEST_SRCS
#		${LCU_ROOT_DIR}/test/file_util_test.c
#		${LCU_ROOT_DIR}/test/file_logger_test.c
#		${LCU_ROOT_DIR}/test/main.c
#)
file(GLOB_RECURSE LCU_TEST_SRCS "${LCU_ROOT_DIR}/test/*.c")
message(STATUS "\n LCU_TEST_SRCS => ${LCU_TEST_SRCS}\n ")

add_executable(libcutils_test
		${LCU_TEST_SRCS}
		)

if(ANDROID)
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
else()
set(log-lib "")
endif(ANDROID)		

target_link_libraries( # Specifies the target library.
        lcu_static PUBLIC

        ${log-lib}
        )
		
target_link_libraries( # Specifies the target library.
        lcu_shared PUBLIC

        ${log-lib}
        )		

target_link_libraries( # Specifies the target library.
        libcutils_test

		lcu_static

        ${log-lib}
        )		

