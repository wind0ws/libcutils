call setup_env.bat %*
if %ERRORLEVEL% NEQ 0 (
  @echo error on setup_env, check it.
  @exit /b 2
) 

@ECHO OFF &PUSHD %~DP0 &TITLE make_windows &color 0A

:lable_check_thread_mode
set WIN_PTHREAD_MODE=%3
if "%WIN_PTHREAD_MODE%" EQU "" (
  @echo now you should choose windows pthread_mode.
  goto label_choose_win_pthread
) else (
  @echo your windows pthread_mode is %WIN_PTHREAD_MODE%
  goto label_select_vs
)

:label_choose_win_pthread
@echo "LCU support 3 type win pthread mode:"
@echo "  0: use windows native implemention."
@echo "  1: use pthreads-win32 lib."
@echo "  2: use pthreads-win32 dll."
@echo .
set /p WIN_PTHREAD_MODE="please choose LCU pthread mode:"

:label_select_vs
set _TMP_VS_VER=%~3
set VS_VER=%3
if "%_TMP_VS_VER:~0,14%" == "Visual Studio " (
  @echo "ok, the No.3 param is %VS_VER%"
) else (
  @echo "oops, No.3 param is not visual studio! use No.4 param!"
  set _TMP_VS_VER=%~4
  set VS_VER=%4
)

@echo VS_VER=%VS_VER%
if "%_TMP_VS_VER%" EQU "" (
  @echo now auto detect vs version for you...
) else (
  @echo use your vs_version=%VS_VER%
  goto label_check_params
)


@echo =============== auto detect VS version ===============
reg query "HKEY_CLASSES_ROOT\VisualStudio.DTE" >> nul 2>&1 
if %ERRORLEVEL% NEQ 0 (
  @echo VS not installed. you should install it first before compile.
  @exit /b 2
) 

@echo VS has installed, now detect newest version.

reg query "HKEY_CLASSES_ROOT\VisualStudio.DTE.17.0" >> nul 2>&1 
if %ERRORLEVEL% NEQ 0 (
  @echo VS 2022 not installed.
) else (
  @echo VS 2022 installed.
  set VS_VER="Visual Studio 17 2022"
  goto label_check_params
) 
reg query "HKEY_CLASSES_ROOT\VisualStudio.DTE.16.0" >> nul 2>&1 
if %ERRORLEVEL% NEQ 0 (
  @echo VS 2019 not installed.
) else (
  @echo VS 2019 installed.
  set VS_VER="Visual Studio 16 2019"
  goto label_check_params
)
reg query "HKEY_CLASSES_ROOT\VisualStudio.DTE.15.0" >> nul 2>&1 
if %ERRORLEVEL% NEQ 0 (
  @echo VS 2017 not installed.
) else (
  @echo VS 2017 installed.
  set VS_VER="Visual Studio 15 2017"
  goto label_check_params
)
reg query "HKEY_CLASSES_ROOT\VisualStudio.DTE.14.0" >> nul 2>&1 
if %ERRORLEVEL% NEQ 0 (
  @echo VS 2015 not installed.
) else (
  @echo VS 2015 installed.
  set VS_VER="Visual Studio 14 2015"
  goto label_check_params
)

@echo error: not support your vs version! maybe it too old!
@exit /b 2
::============= VS check complete =========================  


:label_check_params
@echo =============== detect VS version succeed ===============
if "%BUILD_ABI%" EQU "Win32" set NEW_VS_ARCH=" -A Win32" & goto label_main
if "%BUILD_ABI%" EQU "Win64" set NEW_VS_ARCH="" & goto label_main
@echo params check failed: unknown BUILD_ABI=%BUILD_ABI%
@exit /b 3

:label_main
@echo Your BUILD_ABI=%BUILD_ABI%, NEW_VS_ARCH=%NEW_VS_ARCH:"=%
title=%BUILD_ABI%
set BUILD_DIR=.\build\build_%BUILD_ABI%
@echo Your BUILD_DIR=%BUILD_DIR%
@echo Your BUILD_TYPE=%BUILD_TYPE%
@echo Your VS_VER=%VS_VER%
::set output_dir=.\\output\\android\\armeabi-v7a
rmdir /S /Q %BUILD_DIR:"=% 2>nul
mkdir %BUILD_DIR:"=%


::set CMAKE_EXTEND_ARGS=" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DPRJ_WIN_PTHREAD_MODE=%WIN_PTHREAD_MODE% -DBUILD_STATIC_LIBS=ON -DBUILD_SHARED_LIBS=ON -DBUILD_DEMO=ON" 
set "CMAKE_EXTEND_ARGS=%CMAKE_EXTEND_ARGS% -DPRJ_WIN_PTHREAD_MODE=%WIN_PTHREAD_MODE%"
@echo make_windows-CMAKE_EXTEND_ARGS=%CMAKE_EXTEND_ARGS:"=%
:: VS2019 添加 arch 方式与其他版本不同，默认不加 -A 选项就是Win64(而且不能显式的添加Win64)
:: 小提示：%VAR% 最后面加的 :"=  是为了去除变量两边的双引号的，如果要保留就不要加
%CMAKE_BIN% -G %VS_VER% %NEW_VS_ARCH:"=% -H.\ -B%BUILD_DIR:"=% %CMAKE_EXTEND_ARGS:"=%
::%CMAKE_BIN% -G "Visual Studio 16 2019" -A Win32 -H.\ -B%BUILD_DIR:"=% %CMAKE_EXTEND_ARGS:"=%

set ERR_CODE=%ERRORLEVEL%
IF %ERR_CODE% NEQ 0 (
   @echo.
   @echo "! Error on generate project: %ERR_CODE% !" 
   @exit /b %ERR_CODE%
)
@echo.
@echo "generate project succeed, now compile it ..."
%CMAKE_BIN% --build %BUILD_DIR:"=% --config %BUILD_TYPE:"=%
set ERR_CODE=%ERRORLEVEL%
IF %ERR_CODE% NEQ 0 (
   @echo "! Error on build project: %ERR_CODE% !"
   @exit /b %ERR_CODE%
)

@echo.
@echo "compile %PLATFORM% %BUILD_ABI% %BUILD_TYPE% finished(%ERR_CODE%). bye bye..."
::@pause>nul
::color 0F
@exit /b %ERR_CODE%
