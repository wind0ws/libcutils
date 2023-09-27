#!/bin/bash

BUILD_SCRIPT=./make_linux.sh
dos2unix $BUILD_SCRIPT
chmod +x $BUILD_SCRIPT

#param 1 for arch
#param 2 for build type
source $BUILD_SCRIPT m32 Release
source $BUILD_SCRIPT m64 Release

echo 
echo ...deploy linux finished...
echo 