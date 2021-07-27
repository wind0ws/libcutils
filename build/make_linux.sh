#!/bin/bash
#Attention: Unix sh file shouldn't use CR LF, just use LF

chmod +x ./setup_env.sh
source ./setup_env.sh 

BUILD_DIR=build_linux_$BUILD_ABI
if [ ! -d "$BUILD_DIR" ];then
	echo "mkdir $BUILD_DIR ..."
	mkdir -p $BUILD_DIR
else 
	echo "$BUILD_DIR already exist, clean it up."
	rm -rf $BUILD_DIR/*
fi

#LCU_OUTPUT_DIR=./output/linux/linux32
cmake -H./ -B./$BUILD_DIR -DCMAKE_BUILD_TYPE="$BUILD_TYPE"              \
                          -DCMAKE_C_FLAGS="$COMPILER_FLAGS"             \
						  -DCMAKE_CXX_FLAGS="$COMPILER_FLAGS"           \
						  -DCMAKE_SHARED_LINKER_FLAGS="$COMPILER_FLAGS" \
						  -DCMAKE_EXE_LINKER_FLAGS="$COMPILER_FLAGS" 							
#						  -DARG_LCU_OUTPUT_DIR="${LCU_OUTPUT_DIR}" 

ERR_CODE=$?
if [ $ERR_CODE -ne 0 ];then                                                                          
    echo "  Error on generate project, ERR=$ERR_CODE  "                                                                 
    exit $ERR_CODE                                                                    
fi       

cmake --build $BUILD_DIR --config $BUILD_TYPE
ERR_CODE=$?
##mkdir -p ${LCU_OUTPUT_DIR}
##cp ./build_linux32/libcutils_test ${LCU_OUTPUT_DIR}/
##cp ./build_linux32/liblcu_a.a ${LCU_OUTPUT_DIR}/
##cp ./build_linux32/liblcu.so ${LCU_OUTPUT_DIR}/

echo 
if [ $ERR_CODE -ne 0 ];then                                                                          
    echo "  Error on build $BUILD_ABI $BUILD_TYPE, ERR=$ERR_CODE  "                                                                 
    exit $ERR_CODE
fi
echo "...Build $BUILD_ABI $BUILD_TYPE finished($ERR_CODE)..."
