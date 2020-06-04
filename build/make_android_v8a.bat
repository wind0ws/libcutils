set ANDROID_SDK=D:\Android\android-sdk
set ANDROID_NDK=D:\Android\android-sdk\ndk\20.0.5594570
for /f %%a in ('dir /a:d /b %ANDROID_SDK%\cmake\') do set cmake_version=%%a
echo "find cmake version %cmake_version%"
set cmake_bin=%ANDROID_SDK%\cmake\%cmake_version%\bin\cmake.exe
set ninja_bin=%ANDROID_SDK%\cmake\%cmake_version%\bin\ninja.exe
set tool_chain_file=%ANDROID_NDK%\build\cmake\android.toolchain.cmake

mkdir build_v8a
%cmake_bin% -H.\ -B.\build_v8a ^
			"-GAndroid Gradle - Ninja" ^
			-DANDROID_ABI=arm64-v8a ^
			-DANDROID_NDK=%ANDROID_NDK% ^
			-DCMAKE_BUILD_TYPE=Relase ^
			-DANDROID_TOOLCHAIN=clang ^
			-DCMAKE_TOOLCHAIN_FILE=%tool_chain_file% ^
			-DCMAKE_MAKE_PROGRAM=%ninja_bin% ^
			-DANDROID_PLATFORM=android-19 ^
			-DANDROID_STL=c++_static
%ninja_bin% -C .\build_v8a
mkdir .\output\android\arm64-v8a
cp .\build_v8a\libcutils_test .\output\android\arm64-v8a\
cp .\build_v8a\liblcu.a .\output\android\arm64-v8a\
cp .\build_v8a\liblcu.so .\output\android\arm64-v8a\

::mkdir build_android_x86
::%cmake_bin% -H.\ -B.\build_android_x86 "-GAndroid Gradle - Ninja" -DANDROID_ABI=x86 -DANDROID_NDK=%ANDROID_NDK% -DCMAKE_BUILD_TYPE=Relase -DCMAKE_MAKE_PROGRAM=%ninja_bin% -DCMAKE_TOOLCHAIN_FILE=%tool_chain_file% "-DCMAKE_C_FLAGS=-g -Og -Wall" "-DCMAKE_CXX_FLAGS=-std=c++11 -fexceptions" "-DANDROID_PLATFORM=android-19" "-DANDROID_STL=c++_static"
::%ninja_bin% -C .\build_android_x86
::mkdir .\output\android\x86
::cp .\build_android_x86\libcutils_test .\output\android\x86\

echo "compile complete\n Press any key to exit..."
pause