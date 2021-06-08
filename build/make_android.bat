call setup_env.bat %*

@ECHO OFF&PUSHD %~DP0 &TITLE android &color 0A

:label_check_params
if "%BUILD_ABI%" EQU "armeabi-v7a" goto label_main
if "%BUILD_ABI%" EQU "arm64-v8a" goto label_main
if "%BUILD_ABI%" EQU "x86" goto label_main
if "%BUILD_ABI%" EQU "x86_64" goto label_main
@echo params check failed: unknown BUILD_ABI=%BUILD_ABI%
@exit /b 2

:label_main
@echo Your BUILD_ABI=%BUILD_ABI%
title=%BUILD_ABI%
set BUILD_DIR=build_android_%BUILD_ABI%
@echo Your BUILD_DIR=%BUILD_DIR%
@echo Your BUILD_TYPE=%BUILD_TYPE%
::set output_dir=.\\output\\android\\armeabi-v7a
rmdir /S /Q %BUILD_DIR:"=% 2>nul
mkdir %BUILD_DIR:"=%
%CMAKE_BIN% -H.\ -B.\%BUILD_DIR:"=%                         ^
			"-GNinja"                                       ^
			-DANDROID_ABI=%BUILD_ABI%                       ^
			-DANDROID_NDK=%ANDROID_NDK%                     ^
			-DCMAKE_BUILD_TYPE=%BUILD_TYPE%                 ^
			-DANDROID_TOOLCHAIN=clang                       ^
			-DCMAKE_TOOLCHAIN_FILE=%ANDROID_TOOLCHAIN_FILE% ^
			-DCMAKE_MAKE_PROGRAM=%NINJA_BIN%                ^
			-DANDROID_PLATFORM=android-19                   ^
			-DANDROID_STL=c++_static 
::			-DARG_LCU_OUTPUT_DIR=%output_dir% 

%NINJA_BIN% -C .\%BUILD_DIR:"=%

::mkdir %output_dir%
::copy /Y .\build_android_v7a\libcutils_test %output_dir%\\
::copy /Y .\build_android_v7a\liblcu_a.a %output_dir%\\
::copy /Y .\build_android_v7a\liblcu.so %output_dir%\\

@echo.
@echo "compile finished. bye bye..."
::@pause>nul
color 0F