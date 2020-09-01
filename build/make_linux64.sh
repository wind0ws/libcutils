#!/bin/bash
#Attention: Unix sh file shouldn't use CR LF, just use LF
if [ ! -d "build_linux64" ];then
	echo "mkdir build_linux64..."
	mkdir -p build_linux64
else 
	echo "build_linux64 already exist, clean it up."
	rm -rf build_linux64/*
fi

cmake -H./ -B./build_linux64 -DCMAKE_BUILD_TYPE=Release
cmake --build ./build_linux64 --config Release

mkdir -p ./output/linux/build_linux64
cp ./build_linux64/libcutils_test ./output/linux/build_linux64/
cp ./build_linux64/liblcu_a.a ./output/linux/build_linux64/
cp ./build_linux64/liblcu.so ./output/linux/build_linux64/

echo .
echo Build finished...
