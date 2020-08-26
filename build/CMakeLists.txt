cmake_minimum_required(VERSION 3.8)

project(libcutils VERSION 1.2.6 LANGUAGES C CXX)
message(STATUS "\n Current PROJECT_VERSION=${PROJECT_VERSION} \n")

# 添加自定义cmake文件目录
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
# 使用utility.cmake
include(utility)

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
# 设置版本号，供下面的lcu.version.h.in使用
set(LCU_VER_MAJOR "${PROJECT_VERSION}")

if(WIN32)
	set(LCU_WIN_PTHREAD_MODE 0)
	if(ARG_LCU_WIN_PTHREAD_MODE)
		message(STATUS "ARG_LCU_WIN_PTHREAD_MODE is set, value is ${ARG_LCU_WIN_PTHREAD_MODE}")
		if(("${ARG_LCU_WIN_PTHREAD_MODE}" STREQUAL  ""))
			message(WARNING "ARG_LCU_WIN_PTHREAD_MODE value is empty.")
		else()
			set(LCU_WIN_PTHREAD_MODE ${ARG_LCU_WIN_PTHREAD_MODE})
		endif()
	else()
		message(STATUS "ARG_LCU_WIN_PTHREAD_MODE is not set.")
	endif()
	
	message(STATUS "LCU_WIN_PTHREAD_MODE => ${LCU_WIN_PTHREAD_MODE}")
endif(WIN32)  


#add compile/optimize options for DEBUG/RELEASE
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall ")
if ( WIN32 AND NOT CYGWIN AND NOT ( CMAKE_SYSTEM_NAME STREQUAL "WindowsStore" ) AND NOT ANDROID)
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /Ot /MT")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Od /MTd /DEBUG")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ot /MT")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /MTd /DEBUG /Zi")
	
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}     /utf-8 /W3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /W3")
else()
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Og")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og")
	
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}     -Wall -Wno-unused-local-typedefs -Wno-unused-function")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-unused-local-typedefs -Wno-unused-function -Wno-write-strings")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fexceptions")
endif ()

# 小知识
# add_compile_options命令添加的编译选项是针对所有编译器的(包括c和c++编译器)，
# 而set命令设置CMAKE_C_FLAGS或CMAKE_CXX_FLAGS变量则是分别只针对c和c++编译器的。
# add_definitions(-DFOO) 这个也是向编译器、源码添加选项，但是这个设计的初衷是为了添加宏定义的
# 所以，如果你要是想向编译器添加选项就用 add_compile_options，否则就用 add_definitions
#
#add_compile_options(-D_CRT_SECURE_NO_WARNINGS)
#add_definitions("-DCURL_STATICLIB")
IF(WIN32)
  if(NOT "${CMAKE_CL_64}" MATCHES "0") 
	message(STATUS "current build for WIN x64")
  	set(WIN_ABI "x64")
  else()
    message(STATUS "current build for WIN x86")
    set(WIN_ABI "x86")	
  endif()
  #supress warnings for VS
  add_compile_options( /wd4819 )
  #string(REPLACE "/w4477" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4477 /wd4819" CACHE STRING "")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4477 /wd4819" CACHE STRING "")
ELSE()
  #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
  # Android 5.0 以上需要在此处设置 PIE
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fPIE -pie")
ENDIF(WIN32)


set(LCU_HEADER_ROOT_DIR ${LCU_ROOT_DIR}/inc)
configure_file(
  ${LCU_HEADER_ROOT_DIR}/config/lcu_version.h.in
  ${CMAKE_BINARY_DIR}/generate/config/lcu_version.h
  @ONLY)

configure_file(
  ${LCU_HEADER_ROOT_DIR}/config/lcu_build_config.h.in
  ${CMAKE_BINARY_DIR}/generate/config/lcu_build_config.h
  @ONLY)

#collect header(*.h) directories
#scan_header_dirs(${LCU_HEADER_ROOT_DIR} LCU_HEADER_DIRS)
#message(STATUS "LCU_HEADER_DIRS => \n ${LCU_HEADER_DIRS}\n")
include_directories(${LCU_HEADER_ROOT_DIR}
					${CMAKE_BINARY_DIR}/generate/)

#for publish library: copy header to output dir

#method1: collect all header files, and copy to output dir. the flaw is can't keep dir structure.
#collect header files, exclude cmake configure_file(.h.in)
#file(GLOB_RECURSE HEADER_FILES "${LCU_ROOT_DIR}/inc/*.h" "${LCU_ROOT_DIR}/inc/*.hpp")
#file(COPY ${HEADER_FILES}  DESTINATION  ${CMAKE_CURRENT_LIST_DIR}/output/include/)

#method2: copy header dir to output dir, and remove cmake configure_file, and copy generate configure_file.
file(COPY ${LCU_HEADER_ROOT_DIR} DESTINATION ${CMAKE_CURRENT_LIST_DIR}/output/)
file(GLOB_RECURSE OUT_H_IN_FILES ${CMAKE_CURRENT_LIST_DIR}/output/inc/config/*.h.in)
file(REMOVE ${OUT_H_IN_FILES})
file(GLOB_RECURSE GENERATE_H_FILES ${CMAKE_BINARY_DIR}/generate/*.h)
file(COPY ${GENERATE_H_FILES} DESTINATION ${CMAKE_CURRENT_LIST_DIR}/output/inc/config/)
file(COPY ${LCU_ROOT_DIR}/libs/ DESTINATION ${CMAKE_CURRENT_LIST_DIR}/output/libs/)

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
set_target_properties(lcu_static PROPERTIES OUTPUT_NAME "lcu_a")
set_target_properties(lcu_shared PROPERTIES OUTPUT_NAME "lcu")
# 设置属性CLEAN_DIRECT_OUTPUT可以使动态库和静态库同时存在
#set_target_properties(lcu_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
#set_target_properties(lcu_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
# 指定动态库版本 VERSION 动态库版本 SOVERSION API版本
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
		
#============================================================================SOURCE_GROUP BEGIN		
# make Source tree as same as your folder structure.		
# Reference: https://stackoverflow.com/a/56496104 		
# Create the source groups for source tree with root at LCU_ROOT_DIR. 
# Support from cmake 3.8
source_group(TREE ${LCU_ROOT_DIR} FILES ${LCU_SRCS})
source_group(TREE ${LCU_ROOT_DIR} FILES ${LCU_TEST_SRCS})

#SET(HEADER_FILE_LIST "")
#file(GLOB_RECURSE TMP_HEADERS ${CMAKE_BINARY_DIR}/generate/*.h)
#SET(HEADER_FILE_LIST ${HEADER_FILE_LIST} ${TMP_HEADERS})
#FOREACH (header_dir ${LCU_HEADER_DIRS})
#   file(GLOB_RECURSE TMP_HEADERS ${header_dir}/*.h)
#   SET(HEADER_FILE_LIST ${HEADER_FILE_LIST} ${TMP_HEADERS})
#ENDFOREACH ()
#LIST(REMOVE_DUPLICATES HEADER_FILE_LIST)
#message(STATUS "HEADER_FILE_LIST => \n ${HEADER_FILE_LIST}\n")
#source_group(TREE ${LCU_ROOT_DIR} PREFIX "inc" FILES ${HEADER_FILE_LIST})

#file(GLOB_RECURSE project_headers *.h)
#file(GLOB_RECURSE project_cpps *.cpp)
#set(all_files ${project_headers} ${project_cpps})
#set(all_files ${HEADER_FILE_LIST} ${LCU_TEST_SRCS} ${LCU_SRCS})
#其中all_files是保存了所有文件名的变量。注意，这里用的是变量名，而没有引用其值。
#source_group_by_dir(all_files)
#============================================================================SOURCE_GROUP END	

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

IF(UNIX)
  message(STATUS "Current build system is unix!!!")
  find_package(Threads REQUIRED)
  set(pthread-lib ${CMAKE_THREAD_LIBS_INIT})
  #target_link_libraries(my_app Threads::Threads)
ELSEIF(WIN32)
  message(STATUS "Current build system is WIN32!!!")
  if("${LCU_WIN_PTHREAD_MODE}" MATCHES "0")
	 message(STATUS "use pthread_win_lib")
	 #find_library(pthread-lib pthread_win_lib)
     #set(pthread-lib ${pthread_win_lib})
	 set(pthread-lib "${LCU_ROOT_DIR}/libs/win/${WIN_ABI}/pthreadVC2.lib")
	 
	 #copy_file_on_post_build(libcutils_test "${LCU_ROOT_DIR}/libs/win/${WIN_ABI}/pthreadVC2.dll")
	 #copy_file_on_post_build_to_all_targets("${LCU_ROOT_DIR}/libs/win/${WIN_ABI}/pthreadVC2.dll")
	 #set(dll_files "${LCU_ROOT_DIR}/libs/win/${WIN_ABI}/pthreadVC2.dll" "${LCU_ROOT_DIR}/libs/win/${WIN_ABI}/pthreadGC2.dll")
	 set(dll_files "${LCU_ROOT_DIR}/libs/win/${WIN_ABI}/pthreadVC2.dll")
	 message(STATUS "dll_files=>${dll_files}")
	 #这里用的是变量名，而没有引用其值。因为他是macro，在macro里会对其解引用
	 copy_file_on_post_build_to_all_targets(dll_files)
  else()
     set(pthread-lib "")
  endif()
ELSE()
  set(pthread-lib "")
ENDIF(UNIX)	

message(STATUS "pthread-lib now is: ${pthread-lib}")	

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
		#${pthread-lib}
		#${log-lib}
        )		

