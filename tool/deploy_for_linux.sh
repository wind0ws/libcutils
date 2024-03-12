#!/bin/bash

BUILD_SCRIPT=./make_cross_platform.sh
dos2unix $BUILD_SCRIPT
chmod +x $BUILD_SCRIPT

#param 1 for platform
#param 2 for arch
#param 3 for build type
source $BUILD_SCRIPT linux m64 Release
source $BUILD_SCRIPT linux m32 Release

echo 
echo ...deploy linux finished...
echo
 