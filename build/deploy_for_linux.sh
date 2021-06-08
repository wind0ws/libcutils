#!/bin/bash

#param 1 for arch
#param 2 for build type
source ./make_linux.sh m32 Release
source ./make_linux.sh m64 Release

echo 
echo ...deploy linux finished...