
set(UNIX TRUE CACHE BOOL "")
SET(CMAKE_SYSTEM_NAME Linux) # this one is important
SET(CMAKE_SYSTEM_VERSION 1)  # this one not so much
SET(PLATFORM rk3308)

#R328 STAGING_DIR. (required by toolchain gcc)
#set(ENV{STAGING_DIR} /mnt/d/env/r328/sdk_target)
set(CROSS_TOOLCHAIN_PATH_PREFIX "/mnt/d/env/rk3308/rk3308-arm64-glibc-2018.03-toolschain/usr/bin/aarch64-rockchip-linux-gnu-")

message(STATUS "Current CROSS_TOOLCHAIN_PATH_PREFIX is => ${CROSS_TOOLCHAIN_PATH_PREFIX}")
if(("x${CROSS_TOOLCHAIN_PATH_PREFIX}" STREQUAL  "x"))
  message(FATAL_ERROR  "you should set CROSS_TOOLCHAIN_PATH_PREFIX first")
endif()

#set compiler location
set(CMAKE_C_COMPILER "${CROSS_TOOLCHAIN_PATH_PREFIX}gcc")
set(CMAKE_CXX_COMPILER "${CROSS_TOOLCHAIN_PATH_PREFIX}g++")
set(CMAKE_AR "${CROSS_TOOLCHAIN_PATH_PREFIX}ar")
set(CMAKE_LINKER "${CROSS_TOOLCHAIN_PATH_PREFIX}ld")
set(CMAKE_RANLIB "${CROSS_TOOLCHAIN_PATH_PREFIX}ranlib")
set(CMAKE_NM "${CROSS_TOOLCHAIN_PATH_PREFIX}nm")
set(CMAKE_OBJDUMP "${CROSS_TOOLCHAIN_PATH_PREFIX}objdump")
set(CMAKE_OBJCOPY "${CROSS_TOOLCHAIN_PATH_PREFIX}objcopy")
set(CMAKE_STRIP "${CROSS_TOOLCHAIN_PATH_PREFIX}strip")

string(APPEND CMAKE_C_FLAGS          " -fPIC -mcpu=cortex-a35+crc+crypto")
string(APPEND CMAKE_CXX_FLAGS        " -fPIC -mcpu=cortex-a35+crc+crypto")
string(APPEND CMAKE_EXE_LINKER_FLAGS " -fPIC -fPIE")

# sysroot location
#set(MYSYSROOT /path/to/sysroots/cortexa7-vfp-neon-telechips-linux-gnueabi)
#set(MYSYSROOT "/mnt/d/env/rk3308/rk3308-arm64-glibc-2018.03-toolschain")
# compiler/linker flags
# -march=armv7 -pipe -fomit-frame-pointer
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -fomit-frame-pointer" CACHE INTERNAL "" FORCE)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -fomit-frame-pointer" CACHE INTERNAL "" FORCE)
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --sysroot=${MYSYSROOT}" CACHE INTERNAL "" FORCE)
#set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} --sysroot=${MYSYSROOT}" CACHE INTERNAL "" FORCE)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --sysroot=${MYSYSROOT}" CACHE INTERNAL "" FORCE)
#set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} --sysroot=${MYSYSROOT}" CACHE INTERNAL "" FORCE)
# cmake built-in settings to use find_xxx() functions
#set(CMAKE_FIND_ROOT_PATH "${MYSYSROOT}")
#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

