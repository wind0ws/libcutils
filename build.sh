#!/bin/bash
if [ ! -d cmake_build ]; then
    mkdir cmake_build
fi

cd cmake_build

rm -rf *

cmake ../src 
make

cd ..