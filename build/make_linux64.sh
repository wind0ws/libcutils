#!/bin/bash
#Attention: Unix sh file shouldn't use CR LF, just use LF
if [ ! -d "build_linux64" ];then
	echo "mkdir build_linux64..."
	mkdir -p build_linux64
else 
	echo "build_linux64 already exist, clean it up."
	rm -rf build_linux64/*
fi

LCU_OUTPUT_DIR=./output/linux/linux64

cmake -H./ -B./build_linux64 -DCMAKE_BUILD_TYPE=Release -DARG_LCU_OUTPUT_DIR="${LCU_OUTPUT_DIR}" 
cmake --build ./build_linux64 --config Release

mkdir -p ${LCU_OUTPUT_DIR}
cp ./build_linux64/libcutils_test ${LCU_OUTPUT_DIR}/
cp ./build_linux64/liblcu_a.a ${LCU_OUTPUT_DIR}/
cp ./build_linux64/liblcu.so ${LCU_OUTPUT_DIR}/

echo .
echo Build finished...
