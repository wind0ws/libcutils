#!/bin/bash

BUILD_SCRIPT=./make_cross_platform.sh
dos2unix $BUILD_SCRIPT
chmod +x $BUILD_SCRIPT

_build_type=$1
if [ "${_build_type}" == "" ]; then
  _build_type=Release
fi
echo BUILD_TYPE=$_build_type

#param 1 for platform
#param 2 for arch
#param 3 for build type
source $BUILD_SCRIPT linux m64 $_build_type
source $BUILD_SCRIPT linux m32 $_build_type

echo 
echo ...deploy linux($_build_type) finished...
echo
 