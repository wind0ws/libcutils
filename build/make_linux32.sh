#!/bin/bash
#Attention: Unix sh file shouldn't use CR LF, just use LF
if [ ! -d "build_linux32" ];then
	echo "mkdir build_linux32..."
	mkdir -p build_linux32
else 
	echo "build_linux32 already exist, clean it up."
	rm -rf build_linux32/*
fi

LCU_OUTPUT_DIR=./output/linux/linux32

cmake -H. -B./build_linux32 -DCMAKE_BUILD_TYPE=Release \
                            -DCMAKE_C_FLAGS="-m32" \
							-DCMAKE_CXX_FLAGS="-m32" \
							-DCMAKE_SHARED_LINKER_FLAGS="-m32" \
							-DCMAKE_EXE_LINKER_FLAGS="-m32" 							
#							-DARG_LCU_OUTPUT_DIR="${LCU_OUTPUT_DIR}" 

cmake --build build_linux32 --config Release

mkdir -p ${LCU_OUTPUT_DIR}
cp ./build_linux32/libcutils_test ${LCU_OUTPUT_DIR}/
cp ./build_linux32/liblcu_a.a ${LCU_OUTPUT_DIR}/
cp ./build_linux32/liblcu.so ${LCU_OUTPUT_DIR}/

echo .
echo Build finished...
