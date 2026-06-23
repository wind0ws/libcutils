cmake_minimum_required(VERSION 3.10)

project(lcu VERSION 1.9.0 LANGUAGES C CXX)
message(STATUS "\n Current PROJECT_VERSION=${PROJECT_VERSION}, build_type=${CMAKE_BUILD_TYPE} \n")

# C2 修复: enable_testing() 必须在顶层（不能在 if(BUILD_DEMO) 子作用域内）
enable_testing()

# 添加自定义cmake文件目录
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchains)
include(CheckSymbolExists)
include(CheckIncludeFile)
include(macros)

# P2-6: 探测 C11 <threads.h> 可用性 (cross-compile 下 musl/uclibc 可能缺失).
# 源码侧用 LCU_HAVE_C11_THREADS_H + __has_include 双保险.
check_include_file("threads.h" LCU_HAVE_C11_THREADS_H)
if(LCU_HAVE_C11_THREADS_H)
    add_compile_definitions(LCU_HAVE_C11_THREADS_H=1)
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

message(STATUS "\n current CMakeLists.txt on => ${CMAKE_CURRENT_LIST_DIR}\n")
#get prj root dir, because CMakeLists.txt is in "tool" dir, not in root dir. we should back to parent folder.
get_filename_component(PRJ_ROOT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)
message(STATUS "\n CMAKE_CURRENT_SOURCE_DIR => ${CMAKE_CURRENT_SOURCE_DIR}\n PRJ_ROOT_DIR => ${PRJ_ROOT_DIR}\n")

#=========================== get git info START ===========================
# 获取当前的GIT_HASH
set(PRJ_VER_REVISION "unknown")
get_git_hash(PRJ_VER_REVISION ${PRJ_ROOT_DIR})
message(STATUS "Git hash is ${PRJ_VER_REVISION}")
# 获取当前的分支
set(PRJ_VER_BRANCH "unknown")
get_git_branch(PRJ_VER_BRANCH ${PRJ_ROOT_DIR})
message(STATUS "Git branch is ${PRJ_VER_BRANCH}")
# 设置版本号，供下面的configure_file _version.h.in使用
set(PRJ_VER_MAJOR "${PROJECT_VERSION}")
#=========================== get git info END ===========================

#=========================== detect external params START ===========================
if(WIN32)
    if(("x${PRJ_WIN_PTHREAD_MODE}" STREQUAL  "x"))
       message(WARNING "PRJ_WIN_PTHREAD_MODE value is empty. set to default.")
       set(PRJ_WIN_PTHREAD_MODE 0)
    endif()
    message(STATUS "PRJ_WIN_PTHREAD_MODE => ${PRJ_WIN_PTHREAD_MODE}")
endif(WIN32)  

IF(DEFINED PRJ_OUTPUT_DIR_RELATIVE)
    message(STATUS "detect PRJ_OUTPUT_DIR_RELATIVE => ${PRJ_OUTPUT_DIR_RELATIVE}")
    if(NOT ("x${PRJ_OUTPUT_DIR_RELATIVE}" STREQUAL "x"))
      get_filename_component(PRJ_OUTPUT_DIR ${PRJ_OUTPUT_DIR_RELATIVE} ABSOLUTE)
    endif()
ENDIF()
IF(NOT DEFINED PRJ_OUTPUT_DIR)
  set(PRJ_OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/deploy")
ENDIF()
message(STATUS "current PRJ_OUTPUT_DIR => ${PRJ_OUTPUT_DIR}")
#=========================== detect external params END ===========================

#============================= setup output dir START =============================
IF (NOT DEFINED PLATFORM OR ("x${PLATFORM}" STREQUAL "x"))
  if ((NOT ("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")) OR (NOT ("${CMAKE_SYSTEM_NAME}" STREQUAL "Generic")))
    MESSAGE(WARNING "warning: not defined PLATFORM, use CMAKE_SYSTEM_NAME(${CMAKE_SYSTEM_NAME}) as PLATFORM")
    set(PLATFORM ${CMAKE_SYSTEM_NAME})
  else()
    MESSAGE(FATAL_ERROR "PLATFORM can not be empty! current CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}")
  endif()
ELSE()
  MESSAGE(STATUS "your platform is ${PLATFORM}")
ENDIF()
#detect current PLATFORM_ABI
IF(DEFINED PLATFORM_ABI)
  message(STATUS "PLATFORM_ABI(${PLATFORM_ABI}) is already defined.")
ELSEIF(ANDROID)
  set(PLATFORM_ABI ${ANDROID_ABI})
ELSE()
  IF(CMAKE_CL_64)
    if((CMAKE_C_FLAGS MATCHES "m32") OR (CMAKE_SIZEOF_VOID_P EQUAL 4))
      message(STATUS "CL64 detect m32 in flags OR VOID_P == 4")
      set(PLATFORM_ABI x32)
    else()
      message(STATUS "normal x64")
      set(PLATFORM_ABI x64)
    endif()
  ELSE(CMAKE_CL_64)
    if((CMAKE_C_FLAGS MATCHES "m64") OR (CMAKE_SIZEOF_VOID_P EQUAL 8))
      message(STATUS "CL32 detect m64 in flags OR VOID_P == 8")
      set(PLATFORM_ABI x64)
    else()
      message(STATUS "normal x32")
      set(PLATFORM_ABI x32)
    endif()
  ENDIF(CMAKE_CL_64)
ENDIF(DEFINED PLATFORM_ABI)
string(TOLOWER "${PLATFORM}" PLATFORM_TOLOWER)
string(TOLOWER "${PLATFORM_ABI}" PLATFORM_ABI_TOLOWER)
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_TOLOWER)
# get PLATFORM info on code
add_definitions(-D_PLATFORM=${PLATFORM} -D_PLATFORM_ABI=${PLATFORM_ABI})

set(ALL_CONFIG_TYPES "DEBUG" "RELEASE" "RELWITHDEBINFO" "MINSIZEREL")
foreach(cfg ${ALL_CONFIG_TYPES})
  if (DEFINED DEPLOY_DIR)
    message(STATUS "you defined DEPLOY_DIR => ${DEPLOY_DIR}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg} ${DEPLOY_DIR})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${cfg} ${DEPLOY_DIR})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${cfg} ${DEPLOY_DIR})
  else()
    string(TOLOWER "${cfg}" CFG_TYPE_TOLOWER)
    set(output_dir_tmp "${PRJ_OUTPUT_DIR}/${CFG_TYPE_TOLOWER}/${PLATFORM_TOLOWER}_${PLATFORM_ABI_TOLOWER}")
    message(STATUS "setup OUTPUT_DIRECTORY for ${cfg} -> ${output_dir_tmp}")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg} ${output_dir_tmp})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${cfg} ${output_dir_tmp})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${cfg} ${output_dir_tmp})
  endif()
endforeach()
#============================= setup output dir END =============================

#============================= config header START ============================= 
set(PRJ_HEADER_ROOT_DIR ${PRJ_ROOT_DIR}/inc)
configure_file(
  ${PRJ_HEADER_ROOT_DIR}/config/${CMAKE_PROJECT_NAME}_version.h.in
  ${CMAKE_BINARY_DIR}/generate/config/${CMAKE_PROJECT_NAME}_version.h
  @ONLY)

configure_file(
  ${PRJ_HEADER_ROOT_DIR}/config/${CMAKE_PROJECT_NAME}_build_config.h.in
  ${CMAKE_BINARY_DIR}/generate/config/${CMAKE_PROJECT_NAME}_build_config.h
  @ONLY)

#collect header(*.h) directories
#scan_header_dirs(${PRJ_HEADER_ROOT_DIR} PRJ_HEADER_DIRS)
#message(STATUS "PRJ_HEADER_DIRS => \n ${PRJ_HEADER_DIRS}\n")
include_directories(${PRJ_HEADER_ROOT_DIR}
                    ${CMAKE_BINARY_DIR}/generate/)
#============================= config header END ============================= 

#===================== PUBLISH HEADER BEGIN =====================
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
#file(COPY ${PRJ_ROOT_DIR}/lib/ DESTINATION ${PRJ_OUTPUT_DIR}/lib/)
#===================== PUBLISH HEADER END =====================

#===================== COLLECT SRC BEGIN =====================
#collect src to "PRJ_SRCS" 
#aux_source_directory(${PRJ_ROOT_DIR}/src PRJ_SRCS)
set(PRJ_SRC_DIR "${PRJ_ROOT_DIR}/src")
file(GLOB_RECURSE PRJ_SRCS "${PRJ_SRC_DIR}/*.c" "${PRJ_SRC_DIR}/*.cpp" "${PRJ_SRC_DIR}/*.hpp" "${PRJ_SRC_DIR}/*.cxx")
message(STATUS "\n PRJ_SRCS => ${PRJ_SRCS}\n ")


set(PRJ_DEMO_SRC_DIR "${PRJ_ROOT_DIR}/src_demo")
file(GLOB_RECURSE PRJ_DEMO_SRCS "${PRJ_DEMO_SRC_DIR}/*.c" "${PRJ_DEMO_SRC_DIR}/*.cpp" "${PRJ_DEMO_SRC_DIR}/*.hpp")
set(LCU_DIAGNOSTICS_PROBE_SRCS
    "${PRJ_DEMO_SRC_DIR}/debug/diagnostics_probe.c"
    "${PRJ_DEMO_SRC_DIR}/debug/diagnostics_crt_client.c"
    "${PRJ_DEMO_SRC_DIR}/debug/diagnostics_multicrt_probe.c"
    "${PRJ_DEMO_SRC_DIR}/debug/diagnostics_release_static_probe.c")
list(REMOVE_ITEM PRJ_DEMO_SRCS ${LCU_DIAGNOSTICS_PROBE_SRCS})
message(STATUS "\n PRJ_DEMO_SRCS => ${PRJ_DEMO_SRCS}\n ")
#===================== COLLECT SRC END =====================

#==================== SOURCE_GROUP BEGIN ====================
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
#FOREACH (header_dir ${PRJ_HEADER_DIRS})
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
#==================== SOURCE_GROUP   END ====================

#==================== DECLARE TARGET START ==================
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
  #IF (NOT MSVC) # if you want to strip, build with MinSizeRel  
  #  set_target_properties(${target_prj_static} PROPERTIES LINK_FLAGS_RELEASE -s)
  #ENDIF (NOT MSVC)
  
  list(APPEND target_names ${target_prj_static})
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
  #if(NOT CYGWIN)
  #  # This property causes shared libraries on Linux to have the full version
  #  # encoded into their final filename.  We disable this on Cygwin because
  #  # it causes cygz-${PROJECT_VERSION}.dll to be created when cygz.dll
  #  # seems to be the default.
  #  #
  #  # This has no effect with MSVC, on that platform the version info for
  #  # the DLL comes from the resource file win32/zlib1.rc
  #  set_target_properties(${target_prj_shared} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1)
  #endif()
  IF (NOT MSVC) # strip 
    set_target_properties(${target_prj_shared} PROPERTIES LINK_FLAGS_RELEASE -s)
  ENDIF (NOT MSVC)
  
  list(APPEND target_names ${target_prj_shared})
ENDIF (BUILD_SHARED_LIBS)

IF (BUILD_DEMO)
  add_executable(${target_prj_demo}

                 ${PRJ_DEMO_SRCS})
  # demo 内部头（lcu_test_registry.h / lcu_test_args.h / lcu_test_console.h / lcu_test_glob.h）
  target_include_directories(${target_prj_demo} PRIVATE ${PRJ_DEMO_SRC_DIR})
  list(APPEND target_names ${target_prj_demo})
ENDIF (BUILD_DEMO)

if(WIN32 AND MSVC AND BUILD_DEMO AND BUILD_STATIC_LIBS)
  add_executable(lcu_diagnostics_probe
                 ${PRJ_DEMO_SRC_DIR}/debug/diagnostics_probe.c)
  list(APPEND target_names lcu_diagnostics_probe)
  set(_lcu_release_static_lib
      "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE}/${CMAKE_STATIC_LIBRARY_PREFIX}${CMAKE_PROJECT_NAME}_a${CMAKE_STATIC_LIBRARY_SUFFIX}")
  add_executable(lcu_diagnostics_release_static_probe
                 EXCLUDE_FROM_ALL
                 ${PRJ_DEMO_SRC_DIR}/debug/diagnostics_release_static_probe.c)
  target_link_libraries(lcu_diagnostics_release_static_probe
                        PRIVATE "${_lcu_release_static_lib}")
  list(APPEND target_names lcu_diagnostics_release_static_probe)
endif()

if(WIN32 AND MSVC AND BUILD_DEMO AND BUILD_SHARED_LIBS)
  add_library(lcu_diagnostics_crt_client SHARED
              ${PRJ_DEMO_SRC_DIR}/debug/diagnostics_crt_client.c)
  add_executable(lcu_diagnostics_multicrt_probe
                 ${PRJ_DEMO_SRC_DIR}/debug/diagnostics_multicrt_probe.c)
  list(APPEND target_names
       lcu_diagnostics_crt_client
       lcu_diagnostics_multicrt_probe)
endif()

#==================== DECLARE TARGET END ==================			   


#==================== SETUP FLAGS BEGIN ==================

# 小知识
# add_compile_options命令添加的编译选项是针对所有编译器的(包括c和c++编译器)，
# 而set命令设置CMAKE_C_FLAGS或CMAKE_CXX_FLAGS变量则是分别只针对c和c++编译器的。
# add_definitions(-DFOO) 这个也是向编译器、源码添加选项，但是这个设计的初衷是为了添加宏定义的
# 所以，如果你要是想向编译器添加选项就用 add_compile_options，否则就用 add_definitions
#
#add_compile_options(-D_CRT_SECURE_NO_WARNINGS)
#add_definitions("-DCURL_STATICLIB")
if("x${HAVE_PTHREAD_SETNAME_NP}" STREQUAL "x1")
  add_definitions(-DHAVE_PTHREAD_SETNAME_NP=${HAVE_PTHREAD_SETNAME_NP})
endif()
#add compile/optimize options for DEBUG/RELEASE
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall ")
if ( WIN32 AND NOT CYGWIN AND NOT ( CMAKE_SYSTEM_NAME STREQUAL "WindowsStore" ) AND NOT ANDROID)
  # suppress warnings for VS: 
  # C4001：single line comment warning
  # C4711：warning of function 'function' selected for inline expansion
  # C4819: utf8 coding page warning
  # C4820: 'bytes' bytes padding added after construct 'member_name'
  # C5045: compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
  # C5105: winbase.h undefined behavior warning
  add_compile_options( /wd4001 /wd4711 /wd4819 /wd4820 /wd5045 /wd5105 )
  #add optimiziation options
  add_compile_options(/bigobj /nologo /EHsc /GF /MP)
  #solution folder
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  #put ZERO_CHECK/ALL_BUILD in all_targets folder
  set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "all_targets")
  #set vs startup project
  set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${target_prj_demo})
  
  string(APPEND CMAKE_C_FLAGS_RELEASE        " /Ot /MT")
  string(APPEND CMAKE_C_FLAGS_DEBUG          " /MTd")
  string(APPEND CMAKE_CXX_FLAGS_RELEASE      " /Ot /MT")
  string(APPEND CMAKE_CXX_FLAGS_DEBUG        " /MTd /Zi")
  #string(APPEND CMAKE_C_FLAGS_DEBUG   " -fsanitize=address,undefined") 
  #string(APPEND CMAKE_CXX_FLAGS_DEBUG " -fsanitize=address,undefined") 
  
  set(COMMON_C_FLGAS   " /utf-8 /W3 ")
  set(COMMON_CXX_FLAGS "")
  set(COMMON_EXE_LINKER_FLAGS "")
  set(COMMON_SHARED_LINKER_FLAGS "${COMMON_EXE_LINKER_FLAGS}")
else()
  set(COMMON_C_FLGAS " -Wall -Wno-unused-local-typedefs -Wno-unused-function -Wno-comment -Wno-unknown-pragmas -fPIC")
  set(COMMON_CXX_FLAGS " -Wno-write-strings")
  # Android 5.0 以上需要设置 PIE, 为啥不设置-fPIC, 因为 set(POSITION_INDEPENDENT_CODE TRUE)
  set(COMMON_EXE_LINKER_FLAGS "-fPIE")
  set(COMMON_SHARED_LINKER_FLAGS "${COMMON_EXE_LINKER_FLAGS}")
  #add _GNU_SOURCE for pthread_setname_np
  add_definitions(-D_GNU_SOURCE)
endif()

# Android 16KB 页对齐: Android 15 起部分 64 位设备改用 16KB 内存页,
# 链接时需将 max/common-page-size 设为 16384, 否则在 16KB 设备上无法加载 .so.
# 旧版 NDK(如 r16b) 默认 4KB 需手动指定; NDK r28+ 已默认开启此对齐.
# 静态库是归档文件无需页对齐, 故只追加到 EXE/SHARED 链接标志.
if(ANDROID)
  set(_ANDROID_16K_ALIGN_FLAGS "-Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384")
  string(APPEND COMMON_EXE_LINKER_FLAGS " ${_ANDROID_16K_ALIGN_FLAGS}")
  string(APPEND COMMON_SHARED_LINKER_FLAGS " ${_ANDROID_16K_ALIGN_FLAGS}")
endif()

string(APPEND CMAKE_C_FLAGS   "${COMMON_C_FLGAS}")
string(APPEND CMAKE_CXX_FLAGS "${COMMON_C_FLGAS} ${COMMON_CXX_FLGAS}")
string(APPEND CMAKE_EXE_LINKER_FLAGS " ${COMMON_EXE_LINKER_FLAGS}")
string(APPEND CMAKE_SHARED_LINKER_FLAGS " ${COMMON_SHARED_LINKER_FLAGS}")

# replace MD to MT
if (WIN32 AND MSVC)
    set(flag_var
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
            )

    foreach (variable ${flag_var})
        if (${variable} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
        endif ()
    endforeach ()

    foreach(prj_name ${target_names})
        target_compile_definitions(${prj_name} PRIVATE $<$<CONFIG:Debug>:_DEBUG>)
        set_property(TARGET ${prj_name} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        set_property(TARGET ${prj_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${PRJ_OUTPUT_DIR}")
    endforeach()
    #set_property(TARGET ${target_prj_demo} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif (WIN32 AND MSVC)

#==================== SETUP FLAGS END ==================


#==================== FIND DEPENDENCY LIB BEGIN ==================# 
message(STATUS "CMAKE_SYSTEM_LIBRARY_PATH => ${CMAKE_SYSTEM_LIBRARY_PATH}") # <= find_library will find from this path
set(dependency_local_libs "")
IF(UNIX)
  message(STATUS "Current build system is unix!")
  find_package(Threads REQUIRED)
  set(pthread-lib ${CMAKE_THREAD_LIBS_INIT})
  #target_link_libraries(my_app Threads::Threads)
  check_symbol_exists(pthread_setname_np "pthread.h" HAVE_PTHREAD_SETNAME_NP)

  if(ANDROID)
    if ("${PLATFORM_ABI}" MATCHES "x86_64")
	  message(STATUS "it is android-x86_64! let's append CMAKE_SYSTEM_LIBRARY_PATH for fix lib not found error in cmake!")
	  list(APPEND CMAKE_SYSTEM_LIBRARY_PATH "${ANDROID_SYSTEM_LIBRARY_PATH}/usr/lib64")
	  message(STATUS "current CMAKE_SYSTEM_LIBRARY_PATH => ${CMAKE_SYSTEM_LIBRARY_PATH}")
	endif()
    find_library( # Sets the name of the path variable.
            log-lib

            # Specifies the name of the NDK library that you want CMake to locate.
            log)
  else()
    set(log-lib "")
  endif(ANDROID)
ELSEIF(WIN32)
  message(STATUS "current build system is Windows!")
  set(HAVE_PTHREAD_SETNAME_NP 1)
  #set_property(TARGET ${target_prj_demo} APPEND PROPERTY LINK_FLAGS "/NODEFAULTLIB:LIBCMT")
  if(NOT ("x${PRJ_WIN_PTHREAD_MODE}" MATCHES "x0"))
     #find_library(pthread-lib pthread_win_lib)
     #set(pthread-lib ${pthread_win_lib})
	 # /LTCG 链接时代码生成, 由于在链接时生成代码,可以全局地对所有obj进行优化. 一般用于release版的生成. 
	 # 该选项与增量链接是冲突的.开启了链接时代码生成,即便在增量链接开启的情况下,仍然要进行完整链接.
	 #add_compile_options(/LTCG) 
	 if ("x${PRJ_WIN_PTHREAD_MODE}" MATCHES "x1")
	   message(STATUS "use win32 pthread static lib")
       set(pthread-lib "${PRJ_ROOT_DIR}/lib/windows/${PLATFORM_ABI}/pthread_lib.lib")
	   list(APPEND dependency_local_libs "${pthread-lib}" )
	 else()
	   message(STATUS "use win32 pthread dll")
	   set(pthread-lib "${PRJ_ROOT_DIR}/lib/windows/${PLATFORM_ABI}/pthread_dll.lib")
	   list(APPEND dependency_local_libs "${pthread-lib}" "${PRJ_ROOT_DIR}/lib/windows/${PLATFORM_ABI}/pthread_dll.dll")
	 endif()
     #copy_file_on_post_build(${target_prj_demo} "${PRJ_ROOT_DIR}/lib/windows/${PLATFORM_ABI}/pthreadVC2.dll")
     #copy_file_on_post_build_to_all_targets("${PRJ_ROOT_DIR}/lib/windows/${PLATFORM_ABI}/pthreadVC2.dll")
     #list(APPEND dependency_local_libs "${pthread-lib}" "${PRJ_ROOT_DIR}/lib/windows/${PLATFORM_ABI}/pthreadVC2.dll")
  else()
     set(pthread-lib "")
  endif()
  add_definitions(-DHAVE_PTHREAD_SETNAME_NP=${HAVE_PTHREAD_SETNAME_NP})
ELSE()
  set(pthread-lib "")
  set(log-lib "")
ENDIF(UNIX)

list(LENGTH dependency_local_libs dependency_local_libs_length)
message(STATUS "dependency_local_libs(${dependency_local_libs_length}) => ${dependency_local_libs}")
IF (dependency_local_libs_length GREATER 0)
  message(STATUS "will copy dependency_local_libs for you")
  #这里用的是变量名，而没有引用其值。因为他是macro，在macro里会对其解引用
  copy_file_on_post_build_to_all_targets(dependency_local_libs)
ENDIF()
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
    set(demo_link_lib ${target_prj_static})
  else()
    set(demo_link_lib ${target_prj_shared})
  endif (BUILD_STATIC_LIBS)
  target_link_libraries( # Specifies the target library.
          ${target_prj_demo}

          ${demo_link_lib}
          #${target_prj_static}
          #${pthread-lib}
          #${log-lib}
          )
ENDIF (BUILD_DEMO)

if(TARGET lcu_diagnostics_probe)
  target_link_libraries(lcu_diagnostics_probe ${target_prj_static})
endif()
if(TARGET lcu_diagnostics_crt_client)
  target_link_libraries(lcu_diagnostics_crt_client ${target_prj_shared})
  target_link_libraries(lcu_diagnostics_multicrt_probe
                        ${target_prj_shared}
                        lcu_diagnostics_crt_client)
endif()
#================SETUP dependencies of build target END==================

#========================== CTest 集成 BEGIN ==========================
if(BUILD_DEMO)
    # 扫描 src_demo 下所有源文件，提取 LCU_TEST_REGISTER(name, ...) / LCU_TEST_REGISTER_OPTIONAL(name, ...)
    file(GLOB_RECURSE _test_srcs "${PRJ_DEMO_SRC_DIR}/*.c" "${PRJ_DEMO_SRC_DIR}/*.cpp")
    set(_lcu_tests "")
    set(_lcu_tests_optional "")

    foreach(_file ${_test_srcs})
        # C3: 只匹配未注释的注册宏（行首允许空白，但不允许 //）
        file(STRINGS "${_file}" _lines REGEX "^[ \t]*LCU_TEST_REGISTER(_OPTIONAL)?[ \t]*\\(")
        foreach(_line ${_lines})
            if(_line MATCHES "LCU_TEST_REGISTER_OPTIONAL[ \t]*\\(([A-Za-z_][A-Za-z0-9_]*)")
                list(APPEND _lcu_tests_optional "${CMAKE_MATCH_1}")
            elseif(_line MATCHES "LCU_TEST_REGISTER[ \t]*\\(([A-Za-z_][A-Za-z0-9_]*)")
                list(APPEND _lcu_tests "${CMAKE_MATCH_1}")
            endif()
        endforeach()
    endforeach()
    list(REMOVE_DUPLICATES _lcu_tests)
    list(REMOVE_DUPLICATES _lcu_tests_optional)

    # C6: 工作目录设为 deploy，解决 DLL 路径 + 相对路径问题
    if(CMAKE_CONFIGURATION_TYPES)
        set(_work_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}")
    else()
        set(_work_dir "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endif()
    if(NOT _work_dir)
        set(_work_dir "${CMAKE_BINARY_DIR}")
    endif()

    # 默认用例：进 ctest 默认运行集
    foreach(_name ${_lcu_tests})
        add_test(NAME lcu.${_name} COMMAND ${target_prj_demo} ${_name})
        set_tests_properties(lcu.${_name} PROPERTIES
            LABELS "lcu"
            TIMEOUT 120
            WORKING_DIRECTORY "${_work_dir}")
    endforeach()

    option(LCU_ENABLE_OPTIONAL_TESTS "Register opt-in tests such as intentional leak tests" OFF)

    # Optional tests are registered only when explicitly enabled.
    set(_n_optional 0)
    if(LCU_ENABLE_OPTIONAL_TESTS)
        foreach(_name ${_lcu_tests_optional})
            add_test(NAME lcu.optional.${_name} COMMAND ${target_prj_demo} ${_name})
            set_tests_properties(lcu.optional.${_name} PROPERTIES
                LABELS "lcu_optional"
                TIMEOUT 60
                WORKING_DIRECTORY "${_work_dir}")
        endforeach()
        list(LENGTH _lcu_tests_optional _n_optional)
    endif()

    # 慢用例单独加长 timeout
    foreach(_slow time_util_test file_logger_test)
        if("${_slow}" IN_LIST _lcu_tests)
            set_tests_properties(lcu.${_slow} PROPERTIES TIMEOUT 300)
        endif()
    endforeach()

    # 写共享文件/目录的用例：加 RESOURCE_LOCK 防止并行冲突
    if("file_logger_test" IN_LIST _lcu_tests)
        set_tests_properties(lcu.file_logger_test PROPERTIES RESOURCE_LOCK "log_dir")
    endif()

    # 多线程/共享 logger 的用例：RUN_SERIAL 串行化
    foreach(_serial posix_thread_test thpool_test msg_queue_handler_test)
        if("${_serial}" IN_LIST _lcu_tests)
            set_tests_properties(lcu.${_serial} PROPERTIES RUN_SERIAL TRUE)
        endif()
    endforeach()

    list(LENGTH _lcu_tests _n_default)
    list(LENGTH _lcu_tests_optional _n_optional_discovered)
    message(STATUS "CTest: ${_n_default} default tests + ${_n_optional} optional tests registered (discovered ${_n_optional_discovered}, LCU_ENABLE_OPTIONAL_TESTS=${LCU_ENABLE_OPTIONAL_TESTS})")
endif()
#========================== CTest 集成 END ==========================

set(_lcu_custom_mem_check_enabled OFF)
if(CMAKE_C_FLAGS MATCHES "_LCU_MEM_CHECK_FEATURE_ENABLE" OR
   CMAKE_CXX_FLAGS MATCHES "_LCU_MEM_CHECK_FEATURE_ENABLE")
    set(_lcu_custom_mem_check_enabled ON)
endif()

if(TARGET lcu_diagnostics_probe)
    set(_diagnostics_probe_work_dir
        "${CMAKE_BINARY_DIR}/diagnostics_tests/$<CONFIG>/assert_config")
    add_test(
        NAME lcu.diagnostics.assert_config
        COMMAND ${CMAKE_COMMAND}
            "-DPROBE=$<TARGET_FILE:lcu_diagnostics_probe>"
            "-DMODE=assert_config"
            "-DWORK_DIR=${_diagnostics_probe_work_dir}"
            "-DEXPECT_EXIT=nonzero"
            "-DEXPECT_STDERR=ASSERT"
            "-DEXPECT_LOG_COUNT=1"
            "-DEXPECT_LOG=ASSERT"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_diagnostics_probe.cmake")
    set_tests_properties(lcu.diagnostics.assert_config PROPERTIES
        LABELS "lcu_diagnostics"
        TIMEOUT 10)

    function(add_lcu_diagnostics_probe_test _mode _stderr _stdout _log_count _log)
        set(_probe_args
            "-DPROBE=$<TARGET_FILE:lcu_diagnostics_probe>"
            "-DMODE=${_mode}"
            "-DWORK_DIR=${CMAKE_BINARY_DIR}/diagnostics_tests/$<CONFIG>/${_mode}"
            "-DEXPECT_EXIT=zero"
            "-DEXPECT_LOG_COUNT=${_log_count}")
        if(NOT "${_stderr}" STREQUAL "-")
            list(APPEND _probe_args "-DEXPECT_STDERR=${_stderr}")
        endif()
        if(NOT "${_stdout}" STREQUAL "-")
            list(APPEND _probe_args "-DEXPECT_STDOUT=${_stdout}")
        endif()
        if(NOT "${_log}" STREQUAL "-")
            list(APPEND _probe_args "-DEXPECT_LOG=${_log}")
        endif()
        add_test(
            NAME lcu.diagnostics.${_mode}
            COMMAND ${CMAKE_COMMAND} ${_probe_args}
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_diagnostics_probe.cmake")
        set_tests_properties(lcu.diagnostics.${_mode} PROPERTIES
            CONFIGURATIONS Debug
            LABELS "lcu_diagnostics"
            TIMEOUT 15)
    endfunction()

    if(NOT _lcu_custom_mem_check_enabled)
        add_lcu_diagnostics_probe_test(crt_warn
            "diagnostics probe CRT warning" - 1 "diagnostics probe CRT warning")
        add_lcu_diagnostics_probe_test(crt_leak
            "Detected memory leaks" - 1 "Detected memory leaks")
    endif()
    add_lcu_diagnostics_probe_test(primary_write
        "primary diagnostic" - 1 "primary diagnostic")
    add_lcu_diagnostics_probe_test(concurrency
        "secondary diagnostic" "concurrency probe passed" 1 "secondary diagnostic")
    add_lcu_diagnostics_probe_test(retention
        - "retention probe passed" 32 -)
endif()

if(TARGET lcu_diagnostics_release_static_probe)
    add_test(
        NAME lcu.diagnostics.release_static_debug_client
        COMMAND ${CMAKE_COMMAND}
            "-DBUILD_DIR=${CMAKE_BINARY_DIR}"
            "-DRELEASE_TARGET=${target_prj_static}"
            "-DDEBUG_TARGET=lcu_diagnostics_release_static_probe"
            "-DPROBE=$<TARGET_FILE:lcu_diagnostics_release_static_probe>"
            "-DWORK_DIR=${CMAKE_BINARY_DIR}/diagnostics_tests/release_static_debug_client"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_release_static_debug_client_probe.cmake")
    set_tests_properties(lcu.diagnostics.release_static_debug_client PROPERTIES
        CONFIGURATIONS Debug
        LABELS "lcu_diagnostics"
        TIMEOUT 300)
endif()

if(TARGET lcu_diagnostics_multicrt_probe AND
   NOT _lcu_custom_mem_check_enabled)
    add_test(
        NAME lcu.diagnostics.multicrt
        COMMAND ${CMAKE_COMMAND}
            "-DPROBE=$<TARGET_FILE:lcu_diagnostics_multicrt_probe>"
            "-DMODE=multicrt"
            "-DWORK_DIR=${CMAKE_BINARY_DIR}/diagnostics_tests/$<CONFIG>/multicrt"
            "-DEXPECT_EXIT=zero"
            "-DEXPECT_STDERR=CRT warning"
            "-DEXPECT_STDOUT=multi CRT probe passed"
            "-DEXPECT_LOG_COUNT=1"
            "-DEXPECT_LOG=client DLL CRT warning"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_diagnostics_probe.cmake")
    set_tests_properties(lcu.diagnostics.multicrt PROPERTIES
        CONFIGURATIONS Debug
        LABELS "lcu_diagnostics"
        TIMEOUT 15)
endif()

if(TARGET lcu_diagnostics_probe AND _lcu_custom_mem_check_enabled)
    add_test(
        NAME lcu.diagnostics.tracker_leak
        COMMAND ${CMAKE_COMMAND}
            "-DPROBE=$<TARGET_FILE:lcu_diagnostics_probe>"
            "-DMODE=tracker_leak"
            "-DWORK_DIR=${CMAKE_BINARY_DIR}/diagnostics_tests/$<CONFIG>/tracker_leak"
            "-DEXPECT_EXIT=zero"
            "-DEXPECT_STDERR=MEMORY_LEAK"
            "-DEXPECT_LOG_COUNT=1"
            "-DEXPECT_LOG=MEMORY_LEAK"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_diagnostics_probe.cmake")
    set_tests_properties(lcu.diagnostics.tracker_leak PROPERTIES
        LABELS "lcu_diagnostics"
        TIMEOUT 15)

    add_test(
        NAME lcu.diagnostics.tracker_corruption
        COMMAND ${CMAKE_COMMAND}
            "-DPROBE=$<TARGET_FILE:lcu_diagnostics_probe>"
            "-DMODE=tracker_corruption"
            "-DWORK_DIR=${CMAKE_BINARY_DIR}/diagnostics_tests/$<CONFIG>/tracker_corruption"
            "-DEXPECT_EXIT=nonzero"
            "-DEXPECT_STDERR=MEMORY_CORRUPTION"
            "-DEXPECT_LOG_COUNT=1"
            "-DEXPECT_LOG=MEMORY_CORRUPTION"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/run_diagnostics_probe.cmake")
    set_tests_properties(lcu.diagnostics.tracker_corruption PROPERTIES
        LABELS "lcu_diagnostics"
        TIMEOUT 15)
endif()
