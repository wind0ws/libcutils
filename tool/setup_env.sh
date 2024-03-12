#!/bin/bash
# sh script can't use CRLF, you can use dos2unix to tranform it

echo "show cmake version:"
cmake --version
ERR_CODE=$?
echo 
if [ $ERR_CODE -ne 0 ];then
  echo "  Error on call cmake, ERR=$ERR_CODE, you should install it first!"
  exit $ERR_CODE
fi 

if [ $# -lt 2 ] ; then
  echo "Error: need more param to continue. your param count=$#"
  echo "  sample: $0 linux m64 Release"
  echo "      or: $0 r328 \"\" Release"
  echo "     param1: PLATFORM:   linux, r328 ..."
  echo "     param2: BUILD_ABI:  m32/m64, or just empty string for cross platform"
  echo "     param3: BUILD_TYPE: Debug/Release/MinSizeRel/RelWithDebInfo"
  exit 1
else
  echo "param count=$#"
fi

is_valid_build_type() {  
    local str="$1"  # 获取传入的字符串  
    local list=("Debug" "Release" "MinSizeRel" "RelWithDebInfo") 
  
    for item in "${list[@]}"; do  
        if [[ "$str" == "$item" ]]; then  
            return 0  # 如果找到匹配的字符串，返回状态码0（表示True）  
        fi  
    done  
  
    return 1  # 如果没有找到匹配的字符串，返回状态码1（表示False）  
}  

PLATFORM=linux
BUILD_ABI=
if [ $# -lt 3 ] ; then 
  BUILD_ABI=$1
  BUILD_TYPE=$2
  # 交叉编译兼容2参数形式。第一个参数传平台类型，第二个参数传编译模式
  if [ "$BUILD_ABI" == "m32" -o "$BUILD_ABI" == "m64" ]; then
    echo "BUILD_ABI($BUILD_ABI) is valid"
  else
    PLATFORM=$BUILD_ABI
	BUILD_ABI=
	echo "detect cross PLATFORM=${PLATFORM}"
  fi
else
  PLATFORM=$1
  BUILD_ABI=$2
  BUILD_TYPE=$3
fi 

if is_valid_build_type "$BUILD_TYPE"; then
    echo "your BUILD_TYPE=$BUILD_TYPE"
else
    echo "unknown BUILD_TYPE=$BUILD_TYPE, only support Debug/Release/MinSizeRel/RelWithDebInfo"
    exit 3
fi

COMPILER_FLAGS=""
# 使用数组添加CMake扩展参数  
CMAKE_EXTEND_ARGS=()
if [[ "$PLATFORM" != "linux" ]]; then
  CMAKE_EXTEND_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="./cmake/toolchains/$PLATFORM.toolchain.cmake")  
fi

# 暂未处理交叉编译平台编译时参数，建议放到toolchain里面
if [ "${PLATFORM}" == "linux" ]; then
  echo "your BUILD_ABI=${BUILD_ABI}"
  if [ "${BUILD_ABI}" == "m32" ]; then
    COMPILER_FLAGS=" -m32"
  elif [ "${BUILD_ABI}" == "m64" ]; then
    COMPILER_FLAGS=" -m64"	
  else
    echo "unknown BUILD_ABI=${BUILD_ABI} on linux, only support m32/m64"
    exit 2
  fi
fi

if [[ "${COMPILER_FLAGS}" != "" ]]; then
  CMAKE_EXTEND_ARGS+=(-DCMAKE_C_FLAGS="${COMPILER_FLAGS}")  
  CMAKE_EXTEND_ARGS+=(-DCMAKE_CXX_FLAGS="${COMPILER_FLAGS}")  
  CMAKE_EXTEND_ARGS+=(-DCMAKE_SHARED_LINKER_FLAGS="${COMPILER_FLAGS}")  
  CMAKE_EXTEND_ARGS+=(-DCMAKE_EXE_LINKER_FLAGS="${COMPILER_FLAGS}")  
fi

echo 
echo ===================Your Environment===================
echo
echo PLATFORM=$PLATFORM
echo BUILD_ABI=$BUILD_ABI
echo BUILD_TYPE=$BUILD_TYPE
echo CMAKE_EXTEND_ARGS="${CMAKE_EXTEND_ARGS[@]}" 
echo
echo ======================================================
echo 