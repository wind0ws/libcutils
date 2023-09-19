#!/bin/bash
# sh script can't use CRLF, you can use dos2unix to tranform it

if [ $# -lt 2 ] ; then
  echo "Error: need more param to continue. your param count=$#"
  echo "  sample: $0 m32 Release"
  echo "     param1: BUILD_ABI:  m32 for 32bit, m64 for 64bit"
  echo "     param2: BUILD_TYPE: Debug/Release/MinSizeRel/RelWithDebInfo"
  exit 1
else
  echo "param count=$#"
fi

BUILD_ABI=$1
BUILD_TYPE=$2
COMPILER_FLAGS=""

if [ $BUILD_ABI == "m32" ]; then
    echo "your BUILD_ABI=$BUILD_ABI"
	COMPILER_FLAGS=" -m32"
elif [ $BUILD_ABI == "m64" ]; then
    echo "your BUILD_ABI=$BUILD_ABI"
    COMPILER_FLAGS=" -m64"	
else
    echo "unknown BUILD_ABI=$BUILD_ABI, only support m32/m64"
	exit 2
fi

if [ $BUILD_TYPE == "Debug" -o $BUILD_TYPE == "Release" -o $BUILD_TYPE == "MinSizeRel" -o $BUILD_TYPE == "RelWithDebInfo" ]; then
    echo "your BUILD_TYPE=$BUILD_TYPE"
else
    echo "unknown BUILD_TYPE=$BUILD_TYPE, only support Debug/Release/MinSizeRel/RelWithDebInfo"
	exit 3
fi
