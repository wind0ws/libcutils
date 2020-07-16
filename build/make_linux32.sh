#!/bin/bash
#Attention: Unix sh file shouldn't use CR LF, just use LF
if [ ! -d "build_linux32" ];then
	echo "mkdir build_linux32..."
	mkdir -p build_linux32
else 
	echo "build_linux32 already exist, clean it up."
	rm -rf build_linux32/*
fi

cmake -H. -B./build_linux32 -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_SHARED_LINKER_FLAGS=-m32
cmake --build build_linux32 --config Release

mkdir -p ./output/linux/build_linux32
cp ./build_linux32/libcutils_test ./output/linux/build_linux32/
cp ./build_linux32/liblcu_a.a ./output/linux/build_linux32/
cp ./build_linux32/liblcu.so ./output/linux/build_linux32/
echo Build finished...
