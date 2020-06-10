#!/bin/bash
#Attention: Unix sh file shouldn't use CR LF, just use LF
mkdir -p build_linux32
cmake -H. -B./build_linux32 -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_SHARED_LINKER_FLAGS=-m32
cmake --build build_linux32 --config Release
mkdir -p ./output/linux/build_linux32
cp ./build_linux32/libcutils_test ./output/linux/build_linux32/
cp ./build_linux32/liblcu.a ./output/linux/build_linux32/
cp ./build_linux32/liblcu.so ./output/linux/build_linux32/
echo Build finished...
