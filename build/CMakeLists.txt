cmake_minimum_required(VERSION 3.8)

if ( WIN32 AND NOT CYGWIN AND NOT ( CMAKE_SYSTEM_NAME STREQUAL "WindowsStore" ) AND NOT ANDROID)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT" CACHE STRING "")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd" CACHE STRING "")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT" CACHE STRING "")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd" CACHE STRING "")
endif ()

project(libcutils VERSION 1.0.0 LANGUAGES C CXX)
message(STATUS "\n Current PROJECT_VERSION=${PROJECT_VERSION} \n")

# 添加自定义cmake文件目录
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
# 使用utility.cmake
include(utility)

#set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
#set(CMAKE_C_EXTENSIONS FALSE)

set(CMAKE_VERBOSE_MAKEFILE ON)

message(STATUS "\nCurrent CMakeLists.txt => ${CMAKE_CURRENT_LIST_DIR}\n")
#get lcu root dirs,because CMakeLists.txt is in "build" dir, not in root dir. we should back to parent folder.
get_filename_component(LCU_ROOT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)
message(STATUS "\n CMAKE_CURRENT_SOURCE_DIR => ${CMAKE_CURRENT_SOURCE_DIR}\n LCU_ROOT_DIR=${LCU_ROOT_DIR}\n")

# 获取当前的GIT_HASH
set(GIT_HASH "unknown")
get_git_hash(GIT_HASH ${LCU_ROOT_DIR})
message(STATUS "Git hash is ${GIT_HASH}")

# 获取当前的分支
set(GIT_BRANCH "unknown")
get_git_branch(GIT_BRANCH ${LCU_ROOT_DIR})
message(STATUS "Git branch is ${GIT_BRANCH}")

set(LCU_MAJOR "${PROJECT_VERSION}")

set(LCU_HEADER_ROOT_DIR ${LCU_ROOT_DIR}/inc)
configure_file(
  ${LCU_HEADER_ROOT_DIR}/config/lcu_version.h.in
  ${CMAKE_BINARY_DIR}/generate/lcu_version.h
  @ONLY)

#add compile/optimize options for DEBUG/RELEASE
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall" CACHE STRING "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -fexceptions" CACHE STRING "")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2" CACHE STRING "")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Og" CACHE STRING "")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2" CACHE STRING "")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og" CACHE STRING "")

# 小知识
# add_compile_options命令添加的编译选项是针对所有编译器的(包括c和c++编译器)，
# 而set命令设置CMAKE_C_FLAGS或CMAKE_CXX_FLAGS变量则是分别只针对c和c++编译器的。
# add_definitions(-DFOO) 这个也是向编译器、源码添加选项，但是这个设计的初衷是为了添加宏定义的
# 所以，如果你要是想向编译器添加选项就用 add_compile_options，否则就用 add_definitions
#
#add_compile_options(-D_CRT_SECURE_NO_WARNINGS)
#add_definitions("-DCURL_STATICLIB")
if(WIN32)
#supress warnings for VS
add_compile_options( /wd4819 )
#string(REPLACE "/w4477" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4477 /wd4819" CACHE STRING "")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4477 /wd4819" CACHE STRING "")
else()
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
# Android 5.0 以上需要在此处设置 PIE
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE" CACHE STRING "")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE -pie" CACHE STRING "")
endif(WIN32)

#collect header(*.h) directories
SCAN_HEADER_DIRECTORIES(${LCU_HEADER_ROOT_DIR} LCU_HEADERS)
message(STATUS "LCU_HEADERS => \n ${LCU_HEADERS}\n")
include_directories(${LCU_HEADERS}
					${CMAKE_BINARY_DIR}/generate/)
#include_directories(${LCU_ROOT_DIR}/inc/)

#for publish library: copy header to output dir

#method1: collect all header files, and copy to output dir. the flaw is that can't keep dir structure.
#collect header files, exclude cmake configure_file(.h.in)
#file(GLOB_RECURSE HEADER_FILES "${LCU_ROOT_DIR}/inc/*.h" "${LCU_ROOT_DIR}/inc/*.hpp")
#file(COPY ${HEADER_FILES}  DESTINATION  ${CMAKE_CURRENT_LIST_DIR}/output/include/)

#method2: copy header dir to output dir, and remove cmake configure_file, and copy generate configure_file.
file(COPY ${LCU_HEADER_ROOT_DIR} DESTINATION ${CMAKE_CURRENT_LIST_DIR}/output/)
file(REMOVE ${CMAKE_CURRENT_LIST_DIR}/output/inc/config/lcu_version.h.in)
file(COPY ${CMAKE_BINARY_DIR}/generate/lcu_version.h DESTINATION ${CMAKE_CURRENT_LIST_DIR}/output/inc/config/)

#add_library(mplite ../../src/mplite.c)
#target_include_directories(mplite PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc/)

#collect src to "LCU_SRCS" 
#aux_source_directory(${LCU_ROOT_DIR}/src LCU_SRCS)
file(GLOB_RECURSE LCU_SRCS "${LCU_ROOT_DIR}/src/*.c" "${LCU_ROOT_DIR}/src/*.cpp" "${LCU_ROOT_DIR}/src/*.hpp")
message(STATUS "\n LCU_SRCS => ${LCU_SRCS}\n ")

add_library(lcu_static
			STATIC
			${LCU_SRCS})
add_library(lcu_shared
			SHARED
			${LCU_SRCS})
# 指定库的输出名称(静态库和动态库可以用相同的名称)
set_target_properties(lcu_static PROPERTIES OUTPUT_NAME "lcu")
set_target_properties(lcu_shared PROPERTIES OUTPUT_NAME "lcu")
# 使动态库和静态库同时存在
set_target_properties(lcu_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
set_target_properties(lcu_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
# 指定动态库版本
# VERSION 动态库版本
# SOVERSION API版本
set_target_properties(lcu_shared PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1)

#set(LCU_TEST_SRCS
#		${LCU_ROOT_DIR}/test/file_util_test.c
#		${LCU_ROOT_DIR}/test/file_logger_test.c
#		${LCU_ROOT_DIR}/test/main.c
#)
file(GLOB_RECURSE LCU_TEST_SRCS "${LCU_ROOT_DIR}/test/*.c" "${LCU_ROOT_DIR}/test/*.cpp" "${LCU_ROOT_DIR}/test/*.hpp")
message(STATUS "\n LCU_TEST_SRCS => ${LCU_TEST_SRCS}\n ")
add_executable(libcutils_test
		${LCU_TEST_SRCS}
		)
		
# make Source tree as same as your folder structure.		
# Reference: https://stackoverflow.com/a/56496104 		
# Create the source groups for source tree with root at LCU_ROOT_DIR. 
# Support from cmake 3.8
source_group(TREE ${LCU_ROOT_DIR} FILES ${LCU_SRCS})
source_group(TREE ${LCU_ROOT_DIR} FILES ${LCU_TEST_SRCS})


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

if(UNIX)
message(STATUS "Current system is unix!!!")
find_package(Threads REQUIRED)
set(pthread-lib ${CMAKE_THREAD_LIBS_INIT})
#target_link_libraries(my_app Threads::Threads)
else()
set(pthread-lib "")
endif(UNIX)		

target_link_libraries( # Specifies the target library.
        lcu_static PUBLIC

		${pthread-lib}
        ${log-lib}
        )
		
target_link_libraries( # Specifies the target library.
        lcu_shared PUBLIC

		${pthread-lib}
        ${log-lib}
        )		

target_link_libraries( # Specifies the target library.
        libcutils_test

		lcu_static
        )		

