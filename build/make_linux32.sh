#!/bin/bash
cmake -H. -B./build_linux32 -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 -DCMAKE_SHARED_LINKER_FLAGS=-m32
cmake --build build_linux32 --config Release
echo Build finished...
