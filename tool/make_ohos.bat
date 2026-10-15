call setup_env.bat %*
if %ERRORLEVEL% NEQ 0 (
  @echo error on setup_env, check it.
  @exit /b 2
)

@ECHO OFF &PUSHD %~DP0 &TITLE make_ohos &COLOR 0A

@echo.
@echo detect env OHOS_SDK=%OHOS_SDK%
if "%OHOS_SDK%" EQU "" (
  @echo OHOS_SDK not found on your env, we point a OHOS_SDK path for you.
  set OHOS_SDK=D:\env\hos\sdk_windows\10
  @echo now assume OHOS_SDK=%OHOS_SDK%
)
if not exist "%OHOS_SDK%\" (
    @echo OHOS_SDK not found!
    @exit /b 2
)

set OHOS_TOOLCHAIN_FILE=%OHOS_SDK%\native\build\cmake\ohos.toolchain.cmake
set OHOS_STL=c++_static

if not exist "%OHOS_TOOLCHAIN_FILE%" (
  @echo ERROR: %OHOS_TOOLCHAIN_FILE% not exists!! check your OHOS_SDK installation.
  @exit /b 2
)

@echo.
@echo ==================================================================
@echo OHOS_SDK=%OHOS_SDK%
@echo OHOS_TOOLCHAIN_FILE=%OHOS_TOOLCHAIN_FILE%
@echo OHOS_STL=%OHOS_STL%
@echo ==================================================================
@echo.


:label_check_params
if "%BUILD_ABI%" EQU "armeabi-v7a" goto label_main
if "%BUILD_ABI%" EQU "arm64-v8a" goto label_main
if "%BUILD_ABI%" EQU "x86_64" goto label_main
@echo params check failed: unknown BUILD_ABI=%BUILD_ABI%
@echo supported OHOS ABIs: armeabi-v7a, arm64-v8a, x86_64
@exit /b 2

:label_main
@echo Your BUILD_ABI=%BUILD_ABI%
set BUILD_DIR=.\build\build_ohos_%BUILD_ABI%
TITLE=%BUILD_DIR%
@echo Your BUILD_DIR=%BUILD_DIR%
@echo Your BUILD_TYPE=%BUILD_TYPE%
@echo Your OHOS_STL=%OHOS_STL%
::set OUTPUT_DIR=.\\output\\ohos\\armeabi-v7a
rmdir /S /Q "%BUILD_DIR:"=%" 2>nul
mkdir "%BUILD_DIR:"=%"

%CMAKE_BIN% -H.\ -B%BUILD_DIR:"=%                           ^
            "-GNinja"                                         ^
            -DPLATFORM=ohos                                   ^
            -DPLATFORM_ABI=%BUILD_ABI%                        ^
            -DOHOS_ARCH=%BUILD_ABI%                           ^
            -DOHOS_STL=c++_static                             ^
            -DCMAKE_BUILD_TYPE=%BUILD_TYPE%                   ^
            -DCMAKE_TOOLCHAIN_FILE=%OHOS_TOOLCHAIN_FILE%     ^
            -DCMAKE_MAKE_PROGRAM=%NINJA_BIN%                  ^
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
::copy /Y .\build_ohos_v7a\libcutils_test %OUTPUT_DIR%\\
::copy /Y .\build_ohos_v7a\liblcu_a.a %OUTPUT_DIR%\\
::copy /Y .\build_ohos_v7a\liblcu.so %OUTPUT_DIR%\\

@echo.
@echo "compile finished(%ERR_CODE%). bye bye..."
::@pause>nul
::color 0F
@exit /b %ERR_CODE%
