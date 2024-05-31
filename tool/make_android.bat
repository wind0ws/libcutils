call setup_env.bat %*
if %ERRORLEVEL% NEQ 0 (
  @echo error on setup_env, check it.
  @exit /b 2
) 

@ECHO OFF &PUSHD %~DP0 &TITLE make_android &COLOR 0A

set NDK_VERSION=16.1.4479499

@echo.
@echo detect env ANDROID_NDK=%ANDROID_NDK%
if "%ANDROID_NDK%" EQU "" (
  @echo ANDROID_NDK not found on your env, now detect ANDROID_SDK=%ANDROID_SDK%
  if "%ANDROID_SDK%" NEQ "" (
    set ANDROID_NDK=%ANDROID_SDK%\ndk\%NDK_VERSION%
  ) else (
    @echo oops, ANDROID_SDK not found on your env, we point a ANDROID_NDK path for you.
    set ANDROID_NDK=D:\Android\ndk-multiversion\android-ndk-r16b
  )
  @echo now assume ANDROID_NDK=%ANDROID_NDK%
)
if not exist "%ANDROID_NDK%\" (
    @echo NDK not found!
    @exit /b 2
) 

set ANDROID_TOOLCHAIN_FILE=%ANDROID_NDK%\build\cmake\android.toolchain.cmake
set ANDROID_PLATFORM=android-19 
set ANDROID_STL=c++_static
::set ANDROID_STL=gnustl_static

set PARAM3=%3
if "%PARAM3%" NEQ "" (
  if "%PARAM3%" EQU "c++_static" goto label_set_stl
  if "%PARAM3%" EQU "gnustl_static" goto label_set_stl
  @echo "you provide ANDROID_STL(%PARAM3%), which is NOT supported! available stl are 'c++_static', 'gnustl_static'"
  @exit /b 2
) else goto label_entry

:label_set_stl
set ANDROID_STL=%PARAM3%
@echo use your ANDROID_STL=%ANDROID_STL%

:label_entry
if not exist %ANDROID_TOOLCHAIN_FILE% (
  @echo ERROR: %ANDROID_TOOLCHAIN_FILE% not exists!! should use NDK version greater than or equal r16b.
  @exit /b 2
)

@echo.
@echo ==================================================================
@echo ANDROID_NDK=%ANDROID_NDK%
@echo ANDROID_TOOLCHAIN_FILE=%ANDROID_TOOLCHAIN_FILE%
@echo ANDROID_PLATFORM=%ANDROID_PLATFORM%
@echo ANDROID_STL=%ANDROID_STL%
@echo ==================================================================
@echo.


:label_check_params
if "%BUILD_ABI%" EQU "armeabi-v7a" goto label_main
if "%BUILD_ABI%" EQU "arm64-v8a" goto label_main
if "%BUILD_ABI%" EQU "x86" goto label_main
if "%BUILD_ABI%" EQU "x86_64" goto label_main
if "%BUILD_ABI%" EQU "mips" goto label_main
if "%BUILD_ABI%" EQU "mips64" goto label_main
@echo params check failed: unknown BUILD_ABI=%BUILD_ABI%
@exit /b 2

:label_main
@echo Your BUILD_ABI=%BUILD_ABI%
set BUILD_DIR=.\build\build_android_%BUILD_ABI%
TITLE=%BUILD_DIR%
@echo Your BUILD_DIR=%BUILD_DIR%
@echo Your BUILD_TYPE=%BUILD_TYPE%
@echo Your ANDROID_STL=%ANDROID_STL%
@echo Your ANDROID_PLATFORM=%ANDROID_PLATFORM%
::set OUTPUT_DIR=.\\output\\android\\armeabi-v7a
rmdir /S /Q "%BUILD_DIR:"=%" 2>nul
mkdir "%BUILD_DIR:"=%"

%CMAKE_BIN% -H.\ -B%BUILD_DIR:"=%                           ^
            "-GNinja"                                       ^
            -DANDROID_ARM_NEON=TRUE                         ^
            -DANDROID_ABI=%BUILD_ABI%                       ^
            -DANDROID_NDK=%ANDROID_NDK%                     ^
            -DANDROID_PLATFORM=%ANDROID_PLATFORM%           ^
            -DANDROID_TOOLCHAIN=clang                       ^
            -DANDROID_STL=%ANDROID_STL%                     ^
            -DCMAKE_BUILD_TYPE=%BUILD_TYPE%                 ^
            -DCMAKE_TOOLCHAIN_FILE=%ANDROID_TOOLCHAIN_FILE% ^
            -DCMAKE_MAKE_PROGRAM=%NINJA_BIN%                ^
            %CMAKE_EXTEND_ARGS:"=%
::            -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=ON -DBUILD_DEMO=ON
::          -DPRJ_OUTPUT_DIR_RELATIVE=%OUTPUT_DIR% 

set ERR_CODE=%ERRORLEVEL%
IF %ERR_CODE% NEQ 0 (
   @echo "Error on generate project: %ERR_CODE%"
   @exit /b %ERR_CODE%
)

%NINJA_BIN% -C %BUILD_DIR:"=% -j 8
set ERR_CODE=%ERRORLEVEL%
::mkdir %OUTPUT_DIR%
::copy /Y .\build_android_v7a\libcutils_test %OUTPUT_DIR%\\
::copy /Y .\build_android_v7a\liblcu_a.a %OUTPUT_DIR%\\
::copy /Y .\build_android_v7a\liblcu.so %OUTPUT_DIR%\\

@echo.
@echo "compile finished(%ERR_CODE%). bye bye..."
::@pause>nul
::color 0F
@exit /b %ERR_CODE%
