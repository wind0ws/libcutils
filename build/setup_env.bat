@ECHO OFF &PUSHD %~DP0 &TITLE setup_env &color 0A

set CMAKE_BIN=D:\Env\cmake\bin\cmake.exe

::set ANDROID_SDK=D:\Android\android-sdk
::set ANDROID_NDK=D:\Android\android-sdk\ndk\21.3.6528147
set ANDROID_NDK=D:\Android\ndk-multiversion\android-ndk-r16b
set ANDROID_TOOLCHAIN_FILE=%ANDROID_NDK%\build\cmake\android.toolchain.cmake
::for /f %%a in ('dir /a:d /b %ANDROID_SDK%\cmake\') do set cmake_version=%%a
::echo "find cmake version %cmake_version%"
::set cmake_version=3.10.2.4988404
::set CMAKE_BIN=%ANDROID_SDK%\cmake\%cmake_version%\bin\cmake.exe
::set NINJA_BIN=%ANDROID_SDK%\cmake\%cmake_version%\bin\ninja.exe
set NINJA_BIN=D:\Env\cmake\bin\ninja.exe

@echo ===================Your Environment===================
@echo.
@echo CMAKE_BIN=%CMAKE_BIN%
@echo NINJA_BIN=%NINJA_BIN%
@echo ANDROID_NDK=%ANDROID_NDK%
@echo ANDROID_TOOLCHAIN_FILE=%ANDROID_TOOLCHAIN_FILE%
@echo.
@echo ======================================================


set BUILD_ABI=%1
if "%BUILD_ABI%" EQU "" (
  @echo Now you should input build abi.
  goto label_input_abi
) else (
  @echo your BUILD_ABI is %BUILD_ABI%
  goto label_check_build_type
)

:label_input_abi
@echo "which target abi do you want to build: "
set /p BUILD_ABI="please input BUILD_ABI:"

:label_check_build_type
set BUILD_TYPE=%2
if "%BUILD_TYPE%" EQU "" set BUILD_TYPE=Release
if "%BUILD_TYPE%" EQU "Release" goto label_run_next_bat
if "%BUILD_TYPE%" EQU "Debug" goto label_run_next_bat
@echo unknown BUILD_TYPE=%BUILD_TYPE%, available "Debug"/"Release"
@exit /b 2

:label_run_next_bat
@echo hello...
