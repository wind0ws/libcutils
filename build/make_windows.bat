call setup_env.bat %*

@ECHO OFF &PUSHD %~DP0 &TITLE windows &color 0A

set WIN_PTHREAD_MODE=%3
if "%WIN_PTHREAD_MODE%" EQU "" (
  @echo Now you should choose win pthread mode.
  goto label_choose_win_pthread
) else (
  @echo your pthread_mode is %WIN_PTHREAD_MODE%
  goto label_check_params
)

:label_choose_win_pthread
@echo "LCU support 2 win pthread mode:"
@echo "  0: use pthreads-win32 lib."
@echo "  1: use windows native implemention."
@echo .
set /p WIN_PTHREAD_MODE="please choose LCU pthread mode:"

:label_check_params
if "%BUILD_ABI%" EQU "Win32" set NEW_VS_ARCH=" -A Win32" & goto label_main
if "%BUILD_ABI%" EQU "Win64" set NEW_VS_ARCH="" & goto label_main
@echo params check failed: unknown BUILD_ABI=%BUILD_ABI%
@exit /b 2

:label_main
@echo Your BUILD_ABI=%BUILD_ABI%, NEW_VS_ARCH=%NEW_VS_ARCH:"=%
title=%BUILD_ABI%
set BUILD_DIR=build_%BUILD_ABI%
@echo Your BUILD_DIR=%BUILD_DIR%
@echo Your BUILD_TYPE=%BUILD_TYPE%
::set output_dir=.\\output\\android\\armeabi-v7a
rmdir /S /Q %BUILD_DIR:"=% 2>nul
mkdir %BUILD_DIR:"=%

set CMAKE_EXTEND_ARGS=" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DPRJ_WIN_PTHREAD_MODE=%WIN_PTHREAD_MODE%" 
:: VS2019 添加 arch 方式与其他版本不同，默认不加 -A 选项就是Win64(而且不能显式的添加Win64)
%CMAKE_BIN% -G "Visual Studio 16 2019" %NEW_VS_ARCH:"=% -H.\ -B.\%BUILD_DIR:"=% %CMAKE_EXTEND_ARGS:"=%
::%CMAKE_BIN% -G "Visual Studio 16 2019" -A Win32 -H.\ -B.\%BUILD_DIR:"=% %CMAKE_EXTEND_ARGS:"=%
::-DARG_LCU_OUTPUT_DIR=%output_dir% 
::IF %ERRORLEVEL% NEQ 0 %CMAKE_BIN% -G "Visual Studio 15 2017 %BUILD_ABI:"=%" -H.\ -B.\%BUILD_DIR:"=% %CMAKE_EXTEND_ARGS:"=%
::IF %ERRORLEVEL% NEQ 0 %CMAKE_BIN% -G "Visual Studio 14 2015 %BUILD_ABI:"=%" -H.\ -B.\%BUILD_DIR:"=% %CMAKE_EXTEND_ARGS:"=%
set ERR_CODE=%ERRORLEVEL%
IF %ERR_CODE% NEQ 0 (
   @echo "Error on generate project: %ERR_CODE%"
   @exit /b %ERR_CODE%
)

%CMAKE_BIN% --build %BUILD_DIR:"=% --config %BUILD_TYPE:"=%
set ERR_CODE=%ERRORLEVEL%
::mkdir %output_dir%
::copy /Y .\\build_win32\\Release\\* %output_dir%\\

@echo.
@echo "compile finished. bye bye..."
::@pause>nul
color 0F
@exit /b %ERR_CODE%
