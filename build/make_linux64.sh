#!/bin/bash
#Attention: Unix sh file shouldn't use CR LF, just use LF
mkdir -p build_linux64
cmake -H./ -B./build_linux64
cmake --build ./build_linux64 --config Release

mkdir -p ./output/linux/build_linux64
cp ./build_linux64/libcutils_test ./output/linux/build_linux64/
cp ./build_linux64/liblcu.a ./output/linux/build_linux64/
cp ./build_linux64/liblcu.so ./output/linux/build_linux64/
echo Build finished...
