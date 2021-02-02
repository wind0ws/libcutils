@ECHO OFF&PUSHD %~DP0 &TITLE android_v7a &color 0A
set ANDROID_SDK=D:\Android\android-sdk
::set ANDROID_NDK=D:\Android\android-sdk\ndk\21.3.6528147
set ANDROID_NDK=D:\Android\ndk-multiversion\android-ndk-r16b
::for /f %%a in ('dir /a:d /b %ANDROID_SDK%\cmake\') do set cmake_version=%%a
::echo "find cmake version %cmake_version%"
set cmake_version=3.10.2.4988404
set cmake_bin=%ANDROID_SDK%\cmake\%cmake_version%\bin\cmake.exe
set ninja_bin=%ANDROID_SDK%\cmake\%cmake_version%\bin\ninja.exe
set tool_chain_file=%ANDROID_NDK%\build\cmake\android.toolchain.cmake

set output_dir=.\\output\\android\\armeabi-v7a
rmdir /S /Q build_android_v7a 2>nul
mkdir build_android_v7a
%cmake_bin% -H.\ -B.\build_android_v7a ^
			"-GNinja" ^
			-DANDROID_ABI=armeabi-v7a ^
			-DANDROID_NDK=%ANDROID_NDK% ^
			-DCMAKE_BUILD_TYPE=Release ^
			-DANDROID_TOOLCHAIN=clang ^
			-DCMAKE_TOOLCHAIN_FILE=%tool_chain_file% ^
			-DCMAKE_MAKE_PROGRAM=%ninja_bin% ^
			-DANDROID_PLATFORM=android-19 ^
			-DANDROID_STL=c++_static 
			
::			-DARG_LCU_OUTPUT_DIR=%output_dir% 
%ninja_bin% -C .\build_android_v7a

mkdir %output_dir%
copy /Y .\build_android_v7a\libcutils_test %output_dir%\\
copy /Y .\build_android_v7a\liblcu_a.a %output_dir%\\
copy /Y .\build_android_v7a\liblcu.so %output_dir%\\

@echo.
@echo "Compile complete. Press any key to exit..."
@pause>nul
color 0F