@ECHO OFF&PUSHD %~DP0 &TITLE android_v8a &color 0A
set ANDROID_SDK=D:\Android\android-sdk
::set ANDROID_NDK=D:\Android\android-sdk\ndk\21.3.6528147
set ANDROID_NDK=D:\Android\ndk-multiversion\android-ndk-r16b
::for /f %%a in ('dir /a:d /b %ANDROID_SDK%\cmake\') do set cmake_version=%%a
::echo "find cmake version %cmake_version%"
set cmake_version=3.10.2.4988404
set cmake_bin=%ANDROID_SDK%\cmake\%cmake_version%\bin\cmake.exe
set ninja_bin=%ANDROID_SDK%\cmake\%cmake_version%\bin\ninja.exe
set tool_chain_file=%ANDROID_NDK%\build\cmake\android.toolchain.cmake

rmdir /S /Q build_android_v8a 2>nul
mkdir build_android_v8a
%cmake_bin% -H.\ -B.\build_android_v8a ^
			"-GNinja" ^
			-DANDROID_ABI=arm64-v8a ^
			-DANDROID_NDK=%ANDROID_NDK% ^
			-DCMAKE_BUILD_TYPE=Release ^
			-DANDROID_TOOLCHAIN=clang ^
			-DCMAKE_TOOLCHAIN_FILE=%tool_chain_file% ^
			-DCMAKE_MAKE_PROGRAM=%ninja_bin% ^
			-DANDROID_PLATFORM=android-19 ^
			-DANDROID_STL=c++_static
%ninja_bin% -C .\build_android_v8a
mkdir .\output\android\arm64-v8a
copy /Y .\build_android_v8a\libcutils_test .\output\android\arm64-v8a\
copy /Y .\build_android_v8a\liblcu_a.a .\output\android\arm64-v8a\
copy /Y .\build_android_v8a\liblcu.so .\output\android\arm64-v8a\

@echo.
@echo "compile complete. Press any key to exit..."
@pause>nul
color 0F