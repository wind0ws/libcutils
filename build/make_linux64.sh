#!/bin/bash
cmake -H./ -B./build_linux64
cmake --build ./build_linux64 --config Release
echo Build finished...
