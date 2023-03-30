cmake_minimum_required(VERSION 3.8)

project(lcu VERSION 1.5.3 LANGUAGES C CXX)
message(STATUS "\n Current PROJECT_VERSION=${PROJECT_VERSION}, build_type=${CMAKE_BUILD_TYPE} \n")

# 添加自定义cmake文件目录
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
# 使用utility.cmake
include(utility)
include(CheckSymbolExists)

set(CMAKE_VERBOSE_MAKEFILE ON)
SET(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
#set(CMAKE_C_EXTENSIONS FALSE)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)


option(BUILD_STATIC_LIBS "Build the static library" ON)
option(BUILD_SHARED_LIBS "Build the shared library" ON)
option(BUILD_DEMO "Build demo programs" ON)
message(STATUS "BUILD_STATIC_LIBS=${BUILD_STATIC_LIBS} BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS} BUILD_DEMO=${BUILD_DEMO}\n")
IF(WIN32)
  # for export all symbols on windows 
  # cmake -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE -DBUILD_SHARED_LIBS=TRUE
  option(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS "export dll ALL_SYMBOLS" ON)
ENDIF(WIN32)

#detect current TARGET_ABI
IF(ANDROID)
  set(TARGET_ABI ${ANDROID_ABI})
ELSEIF(WIN32)
  IF(CMAKE_CL_64)
    set(TARGET_ABI x64)
  ELSE(CMAKE_CL_64)
    set(TARGET_ABI x86)
  ENDIF(CMAKE_CL_64)
ELSE()
  IF(CMAKE_C_FLAGS MATCHES "m32")
    set(TARGET_ABI x86)
  ELSE()
    set(TARGET_ABI x64)
  ENDIF()
ENDIF(ANDROID)

message(STATUS "\nCurrent CMakeLists.txt => ${CMAKE_CURRENT_LIST_DIR}\n")
#get lcu root dirs, because CMakeLists.txt is in "build" dir, not in root dir. we should back to parent folder.
get_filename_component(PRJ_ROOT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)
message(STATUS "\n CMAKE_CURRENT_SOURCE_DIR => ${CMAKE_CURRENT_SOURCE_DIR}\n PRJ_ROOT_DIR => ${PRJ_ROOT_DIR}\n")

# 获取当前的GIT_HASH
set(PRJ_VER_REVISION "unknown")
get_git_hash(PRJ_VER_REVISION ${PRJ_ROOT_DIR})
message(STATUS "Git hash is ${PRJ_VER_REVISION}")

# 获取当前的分支
set(PRJ_VER_BRANCH "unknown")
get_git_branch(PRJ_VER_BRANCH ${PRJ_ROOT_DIR})
message(STATUS "Git branch is ${PRJ_VER_BRANCH}")
# 设置版本号，供下面的lcu_version.h.in使用
set(PRJ_VER_MAJOR "${PROJECT_VERSION}")

if(WIN32)
	#set(PRJ_WIN_PTHREAD_MODE 0)
	if(PRJ_WIN_PTHREAD_MODE)
		message(STATUS "PRJ_WIN_PTHREAD_MODE is set, value = ${PRJ_WIN_PTHREAD_MODE}")
		if(("${PRJ_WIN_PTHREAD_MODE}" STREQUAL  ""))
			message(WARNING "PRJ_WIN_PTHREAD_MODE value is empty.")
			set(PRJ_WIN_PTHREAD_MODE 0)
		endif()
	else()
		message(STATUS "PRJ_WIN_PTHREAD_MODE is not set.")
		set(PRJ_WIN_PTHREAD_MODE 0)
	endif(PRJ_WIN_PTHREAD_MODE)
	
	message(STATUS "PRJ_WIN_PTHREAD_MODE => ${PRJ_WIN_PTHREAD_MODE}")
endif(WIN32)  

#set(PRJ_OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/output")
if(PRJ_OUTPUT_DIR)
    message(STATUS "PRJ_OUTPUT_DIR is set, value = ${PRJ_OUTPUT_DIR}")
	if(("${PRJ_OUTPUT_DIR}" STREQUAL  ""))
		message(WARNING "PRJ_OUTPUT_DIR value is empty.")
		set(PRJ_OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/output")
	else()
	    get_filename_component(PRJ_OUTPUT_DIR ${ARG_PRJ_OUTPUT_DIR} ABSOLUTE)
		#set(PRJ_OUTPUT_DIR ${ARG_PRJ_OUTPUT_DIR})
	endif()
else()
   message(STATUS "ARG_PRJ_OUTPUT_DIR is not set. use default")
   set(PRJ_OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/output")
endif(PRJ_OUTPUT_DIR)
message(STATUS "current PRJ_OUTPUT_DIR => ${PRJ_OUTPUT_DIR}")

IF (WIN32)
  set(DEPLOY_DIR "${PRJ_OUTPUT_DIR}/${CMAKE_BUILD_TYPE}/windows")
ELSEIF (ANDROID)
  set(DEPLOY_DIR "${PRJ_OUTPUT_DIR}/${CMAKE_BUILD_TYPE}/android")
ELSE()
  set(DEPLOY_DIR "${PRJ_OUTPUT_DIR}/${CMAKE_BUILD_TYPE}/linux")
ENDIF (WIN32)

set(DEPLOY_DIR "${DEPLOY_DIR}/${TARGET_ABI}")
message(STATUS  "DEPLOY_DIR => ${DEPLOY_DIR}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${DEPLOY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${DEPLOY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${DEPLOY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${DEPLOY_DIR})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${DEPLOY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${DEPLOY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${DEPLOY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL ${DEPLOY_DIR})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${DEPLOY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${DEPLOY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${DEPLOY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL ${DEPLOY_DIR})



set(PRJ_HEADER_ROOT_DIR ${PRJ_ROOT_DIR}/inc)
configure_file(
  ${PRJ_HEADER_ROOT_DIR}/config/lcu_version.h.in
  ${CMAKE_BINARY_DIR}/generate/config/lcu_version.h
  @ONLY)

configure_file(
  ${PRJ_HEADER_ROOT_DIR}/config/lcu_build_config.h.in
  ${CMAKE_BINARY_DIR}/generate/config/lcu_build_config.h
  @ONLY)

#collect header(*.h) directories
#scan_header_dirs(${PRJ_HEADER_ROOT_DIR} LCU_HEADER_DIRS)
#message(STATUS "LCU_HEADER_DIRS => \n ${LCU_HEADER_DIRS}\n")
include_directories(${PRJ_HEADER_ROOT_DIR}
					${CMAKE_BINARY_DIR}/generate/)

#=====================PUBLISH HEADER BEGIN=====================
#method1: collect all header files, and copy to output dir. the flaw is can't keep dir structure.
#collect header files, exclude cmake configure_file(.h.in)
#file(GLOB_RECURSE HEADER_FILES "${PRJ_ROOT_DIR}/inc/*.h" "${PRJ_ROOT_DIR}/inc/*.hpp")

#file(COPY ${HEADER_FILES}  DESTINATION  ${CMAKE_CURRENT_LIST_DIR}/output/include/)

#method2: copy header dir to output dir, and remove cmake configure_file, and copy generate configure_file.
file(COPY ${PRJ_HEADER_ROOT_DIR} DESTINATION ${PRJ_OUTPUT_DIR}/)
file(GLOB_RECURSE OUT_H_IN_FILES ${PRJ_OUTPUT_DIR}/inc/config/*.h.in)
file(REMOVE ${OUT_H_IN_FILES})
file(GLOB_RECURSE GENERATE_H_FILES ${CMAKE_BINARY_DIR}/generate/*.h)
file(COPY ${GENERATE_H_FILES} DESTINATION ${PRJ_OUTPUT_DIR}/inc/config/)
#file(COPY ${PRJ_ROOT_DIR}/libs/ DESTINATION ${PRJ_OUTPUT_DIR}/libs/)
#=====================PUBLISH HEADER END=====================

#=====================COLLECT SRC BEGIN=====================
#collect src to "PRJ_SRCS" 
#aux_source_directory(${PRJ_ROOT_DIR}/src PRJ_SRCS)
set(PRJ_SRC_DIR "${PRJ_ROOT_DIR}/src")
file(GLOB_RECURSE PRJ_SRCS "${PRJ_SRC_DIR}/*.c" "${PRJ_SRC_DIR}/*.cpp" "${PRJ_SRC_DIR}/*.hpp" "${PRJ_SRC_DIR}/*.cxx")
message(STATUS "\n PRJ_SRCS => ${PRJ_SRCS}\n ")

#set(PRJ_DEMO_SRCS
#		${PRJ_ROOT_DIR}/test/file_util_test.c
#		${PRJ_ROOT_DIR}/test/file_logger_test.c
#		${PRJ_ROOT_DIR}/test/main.c
#)
set(PRJ_DEMO_SRC_DIR "${PRJ_ROOT_DIR}/test")
file(GLOB_RECURSE PRJ_DEMO_SRCS "${PRJ_DEMO_SRC_DIR}/*.c" "${PRJ_DEMO_SRC_DIR}/*.cpp" "${PRJ_DEMO_SRC_DIR}/*.hpp")
message(STATUS "\n PRJ_DEMO_SRCS => ${PRJ_DEMO_SRCS}\n ")
#=====================COLLECT SRC END=====================

#====================SOURCE_GROUP BEGIN====================
# make Source tree as same as your folder structure.		
# Reference: https://stackoverflow.com/a/56496104 		
# Create the source groups for source tree with root at PRJ_ROOT_DIR. 
# Support from cmake 3.8
source_group(TREE ${PRJ_ROOT_DIR} FILES ${PRJ_SRCS})
source_group(TREE ${PRJ_ROOT_DIR} FILES ${PRJ_DEMO_SRCS})
#source_group(TREE ${PRJ_ROOT_DIR} FILES ${HEADER_FILES})

#SET(HEADER_FILE_LIST "")
#file(GLOB_RECURSE TMP_HEADERS ${CMAKE_BINARY_DIR}/generate/*.h)
#SET(HEADER_FILE_LIST ${HEADER_FILE_LIST} ${TMP_HEADERS})
#FOREACH (header_dir ${LCU_HEADER_DIRS})
#   file(GLOB_RECURSE TMP_HEADERS ${header_dir}/*.h)
#   SET(HEADER_FILE_LIST ${HEADER_FILE_LIST} ${TMP_HEADERS})
#ENDFOREACH ()
#LIST(REMOVE_DUPLICATES HEADER_FILE_LIST)
#message(STATUS "HEADER_FILE_LIST => \n ${HEADER_FILE_LIST}\n")
#source_group(TREE ${PRJ_ROOT_DIR} PREFIX "inc" FILES ${HEADER_FILE_LIST})

#file(GLOB_RECURSE project_headers *.h)
#file(GLOB_RECURSE project_cpps *.cpp)
#set(all_files ${project_headers} ${project_cpps})
#set(all_files ${HEADER_FILE_LIST} ${PRJ_DEMO_SRCS} ${PRJ_SRCS})
#其中all_files是保存了所有文件名的变量。注意，这里用的是变量名，而没有引用其值。
#source_group_by_dir(all_files)
#====================SOURCE_GROUP   END====================

#====================DECLARE TARGET START==================
set(target_prj_static "${CMAKE_PROJECT_NAME}_static")
set(target_prj_shared "${CMAKE_PROJECT_NAME}_shared")
set(target_prj_demo   "${CMAKE_PROJECT_NAME}_demo")

IF (BUILD_STATIC_LIBS)
  add_library(${target_prj_static}
  			  STATIC
  			  
  			  ${HEADER_FILES}
  			  ${PRJ_SRCS})
  set_target_properties(${target_prj_static} PROPERTIES OUTPUT_NAME "${CMAKE_PROJECT_NAME}_a")
  #set_target_properties(${target_prj_static} PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  IF (NOT MSVC) # strip 
    set_target_properties(${target_prj_static} PROPERTIES LINK_FLAGS_RELEASE -s)
  ENDIF (NOT MSVC)
ENDIF(BUILD_STATIC_LIBS)

IF (BUILD_SHARED_LIBS)			
  add_library(${target_prj_shared}
  			  SHARED
  			  
  			  ${HEADER_FILES}
  			  ${PRJ_SRCS})

  # 指定库的输出名称(静态库和动态库可以用相同的名称)
  set_target_properties(${target_prj_shared} PROPERTIES OUTPUT_NAME "${CMAKE_PROJECT_NAME}")
  # 设置属性CLEAN_DIRECT_OUTPUT可以使动态库和静态库同时存在
  #set_target_properties(${target_prj_shared} PROPERTIES CLEAN_DIRECT_OUTPUT 1)
  # 指定动态库版本 VERSION 动态库版本 SOVERSION API版本
  set_target_properties(${target_prj_shared} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1)
  IF (NOT MSVC) # strip 
    set_target_properties(${target_prj_shared} PROPERTIES LINK_FLAGS_RELEASE -s)
  ENDIF (NOT MSVC)
ENDIF (BUILD_SHARED_LIBS)
			
IF (BUILD_DEMO)			
  add_executable(${target_prj_demo}
  
  			     ${PRJ_DEMO_SRCS})
ENDIF (BUILD_DEMO)

#====================DECLARE TARGET END==================			   


#==================== SETUP FLAGS BEGIN==================
IF(WIN32)
  # suppress warnings for VS: 
  # C4819: suppress utf8 coding page warning
  # C4001：suppress single line comment warning
  # C5105: suppress winbase.h undefined behavior warning
  add_compile_options( /wd4819 /wd4001 /wd5105 )
  #add optimiziation options
  add_compile_options(/bigobj /nologo /EHsc /GF /MP)
  #solution folder
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  #put ZERO_CHECK/ALL_BUILD in all_targets folder
  set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "all_targets")
  #set vs startup project
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${target_prj_demo})
  
  #string(REPLACE "/w4477" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4477 /wd4819" CACHE STRING "")
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4477 /wd4819" CACHE STRING "")
ELSE() # for unix
  #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
  # Android 5.0 以上需要在此处设置 PIE
  string(APPEND CMAKE_C_FLAGS          " -fPIC")
  string(APPEND CMAKE_CXX_FLAGS        " -fPIC")
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -fPIC -fPIE")
  #add _GNU_SOURCE for pthread_setname_np
  add_definitions(-D_GNU_SOURCE)
ENDIF(WIN32)

# 小知识
# add_compile_options命令添加的编译选项是针对所有编译器的(包括c和c++编译器)，
# 而set命令设置CMAKE_C_FLAGS或CMAKE_CXX_FLAGS变量则是分别只针对c和c++编译器的。
# add_definitions(-DFOO) 这个也是向编译器、源码添加选项，但是这个设计的初衷是为了添加宏定义的
# 所以，如果你要是想向编译器添加选项就用 add_compile_options，否则就用 add_definitions
#
#add_compile_options(-D_CRT_SECURE_NO_WARNINGS)
#add_definitions("-DCURL_STATICLIB")
if("${HAVE_PTHREAD_SETNAME_NP}" STREQUAL "1")
  add_definitions(-DHAVE_PTHREAD_SETNAME_NP=${HAVE_PTHREAD_SETNAME_NP})
endif()
#add compile/optimize options for DEBUG/RELEASE
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall ")
if ( WIN32 AND NOT CYGWIN AND NOT ( CMAKE_SYSTEM_NAME STREQUAL "WindowsStore" ) AND NOT ANDROID)
	string(APPEND CMAKE_C_FLAGS_RELEASE        " /O2 /Ot /MT")
	string(APPEND CMAKE_C_FLAGS_DEBUG          " /Od /MTd /DEBUG")
	string(APPEND CMAKE_CXX_FLAGS_RELEASE      " /O2 /Ot /MT")
	string(APPEND CMAKE_CXX_FLAGS_DEBUG        " /Od /MTd /DEBUG /Zi")
	#string(APPEND CMAKE_C_FLAGS_DEBUG   " -fsanitize=address,undefined") 
	#string(APPEND CMAKE_CXX_FLAGS_DEBUG " -fsanitize=address,undefined") 
	
	set(COMMON_C_FLGAS   " /utf-8 /W3")
	set(COMMON_CXX_FLAGS "")
else()
	string( TOLOWER "${CMAKE_BUILD_TYPE}" BUILD_TYPE_LOWER )
    if (BUILD_TYPE_LOWER STREQUAL "debug")
        message(STATUS "Debug build detected")
        add_definitions("-D_DEBUG=1")
    else()
        message(STATUS "Release build detected")
		add_definitions("-DNDEBUG")
    endif()
	string(APPEND CMAKE_C_FLAGS_RELEASE    " -O3")
	string(APPEND CMAKE_C_FLAGS_DEBUG      " -Og")
	string(APPEND CMAKE_CXX_FLAGS_RELEASE  " -O3")
	string(APPEND CMAKE_CXX_FLAGS_DEBUG    " -Og")
	
	set(COMMON_C_FLGAS " -Wall -Wno-unused-local-typedefs -Wno-unused-function -Wno-comment -Wno-unknown-pragmas")
    set(COMMON_CXX_FLAGS " -Wno-write-strings")
endif()
string(APPEND CMAKE_C_FLAGS   "${COMMON_C_FLGAS}")
string(APPEND CMAKE_CXX_FLAGS "${COMMON_C_FLGAS} ${COMMON_CXX_FLGAS}")
#==================== SETUP FLAGS END==================


#==================== FIND DEPENDENCY LIB BEGIN ==================
IF(UNIX)
  message(STATUS "Current build system is unix!")
  find_package(Threads REQUIRED)
  set(pthread-lib ${CMAKE_THREAD_LIBS_INIT})
  #target_link_libraries(my_app Threads::Threads)
  check_symbol_exists(pthread_setname_np "pthread.h" HAVE_PTHREAD_SETNAME_NP)

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
ELSEIF(WIN32)
  message(STATUS "Current build system is WIN32!")
  set(HAVE_PTHREAD_SETNAME_NP 1)
  #set_property(TARGET ${target_prj_demo} APPEND PROPERTY LINK_FLAGS "/NODEFAULTLIB:LIBCMT")
  if("${PRJ_WIN_PTHREAD_MODE}" MATCHES "0")
	 message(STATUS "use pthread_win_lib")
	 #find_library(pthread-lib pthread_win_lib)
     #set(pthread-lib ${pthread_win_lib})
	 set(pthread-lib "${PRJ_ROOT_DIR}/libs/windows/${TARGET_ABI}/pthreadVC2.lib")
	 
	 #copy_file_on_post_build(${target_prj_demo} "${PRJ_ROOT_DIR}/libs/windows/${TARGET_ABI}/pthreadVC2.dll")
	 #copy_file_on_post_build_to_all_targets("${PRJ_ROOT_DIR}/libs/windows/${TARGET_ABI}/pthreadVC2.dll")
	 set(dll_files "${pthread-lib}" "${PRJ_ROOT_DIR}/libs/windows/${TARGET_ABI}/pthreadVC2.dll")
	 message(STATUS "dll_files => ${dll_files}")
	 #这里用的是变量名，而没有引用其值。因为他是macro，在macro里会对其解引用
	 copy_file_on_post_build_to_all_targets(dll_files)
  else()
     set(pthread-lib "")
  endif()
ELSE()
  set(pthread-lib "")
  set(log-lib "")
ENDIF(UNIX)

message(STATUS "HAVE_PTHREAD_SETNAME_NP => ${HAVE_PTHREAD_SETNAME_NP}")
message(STATUS "pthread-lib => ${pthread-lib} , log-lib => ${log-lib}")

#==================== FIND DEPENDENCY LIB END ==================


#================SETUP dependencies of build target BEGIN================
IF (BUILD_STATIC_LIBS)	
  target_link_libraries( # Specifies the target library.
          ${target_prj_static} PUBLIC
  
  		  ${pthread-lib}
          ${log-lib}
          )
ENDIF (BUILD_STATIC_LIBS)

IF (BUILD_SHARED_LIBS)			
  target_link_libraries( # Specifies the target library.
          ${target_prj_shared} PUBLIC
  
  		  ${pthread-lib}
          ${log-lib}
          )
ENDIF (BUILD_SHARED_LIBS)

IF (BUILD_DEMO)
  if (BUILD_STATIC_LIBS)
    set(demo_link_lcu_lib ${target_prj_static})
  else()
	set(demo_link_lcu_lib ${target_prj_shared})
  endif (BUILD_STATIC_LIBS)  
  target_link_libraries( # Specifies the target library.
          ${target_prj_demo}
  
		  ${demo_link_lcu_lib}
  		  #${target_prj_static}
  		  #${pthread-lib}
  		  #${log-lib}
          )
ENDIF (BUILD_DEMO)	
#================SETUP dependencies of build target END==================
