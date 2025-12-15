#SET(CMAKE_SYSTEM_NAME "R328")
set(UNIX TRUE CACHE BOOL "")

#R328 STAGING_DIR. (required by toolchain gcc)
#set(ENV{STAGING_DIR} /mnt/d/Env/r328/sdk_target)

# compiler
set(CROSS_TOOLCHAIN_BIN_DIR "/mnt/d/Env/gcc-linaro-7.5.0-2019.12-x86_64_arm-linux-gnueabihf/bin")
set(CMAKE_C_COMPILER "${CROSS_TOOLCHAIN_BIN_DIR}/arm-linux-gnueabihf-gcc")
set(CMAKE_CXX_COMPILER "${CROSS_TOOLCHAIN_BIN_DIR}/arm-linux-gnueabihf-g++")
set(CMAKE_AR "${CROSS_TOOLCHAIN_BIN_DIR}/arm-linux-gnueabihf-ar")
set(CMAKE_STRIP "${CROSS_TOOLCHAIN_BIN_DIR}/arm-linux-gnueabihf-strip")
set(CMAKE_STRINGS "${CROSS_TOOLCHAIN_BIN_DIR}/arm-linux-gnueabihf-strings")
# sysroot location
#set(MYSYSROOT /path/to/sysroots/cortexa7-vfp-neon-telechips-linux-gnueabi)
#set(MYSYSROOT "/mnt/d/Env/r328/sdk_target")
# compiler/linker flags
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --sysroot=${MYSYSROOT}" CACHE INTERNAL "" FORCE)
#set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} --sysroot=${MYSYSROOT}" CACHE INTERNAL "" FORCE)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --sysroot=${MYSYSROOT}" CACHE INTERNAL "" FORCE)
#set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} --sysroot=${MYSYSROOT}" CACHE INTERNAL "" FORCE)
# cmake built-in settings to use find_xxx() functions
#set(CMAKE_FIND_ROOT_PATH "${MYSYSROOT}")
#set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
#set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
#set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

