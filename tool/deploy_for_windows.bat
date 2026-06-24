@ECHO OFF&PUSHD %~DP0 &TITLE deploy_windows &COLOR 0A

set _build_type=%1
if "%_build_type%" EQU "" (
  set _build_type=Release
)
@echo BUILD_TYPE=%_build_type%

::param 1 for arch: "Win32" or "Win64"
::param 2 for build type: "Debug" / "Release" / "MinSizeRel" / "RelWithDebInfo"
::param 3 for WIN_PTHREAD_MODE: 0 native, 1 pthread_lib, 2 pthread_dll
::param 4 for VS_VER: such as "Visual Studio 17 2022"
:: This script detects every VS installed on the machine and builds
:: each one x {Win32,Win64}. pthread_mode is fixed to 1 (pthreads-win32 static lib).

:: ========== detect all installed VS versions ==========
set "VS_LIST="
reg query "HKEY_CLASSES_ROOT\VisualStudio.DTE.17.0" >nul 2>&1 && set "VS_LIST=%VS_LIST% 17"
reg query "HKEY_CLASSES_ROOT\VisualStudio.DTE.16.0" >nul 2>&1 && set "VS_LIST=%VS_LIST% 16"
reg query "HKEY_CLASSES_ROOT\VisualStudio.DTE.15.0" >nul 2>&1 && set "VS_LIST=%VS_LIST% 15"
reg query "HKEY_CLASSES_ROOT\VisualStudio.DTE.14.0" >nul 2>&1 && set "VS_LIST=%VS_LIST% 14"

if "%VS_LIST%"=="" (
  @echo ERROR: No Visual Studio 2015/2017/2019/2022 detected. Install one first.
  @exit /b 1
)
@echo Detected VS versions:%VS_LIST%

:: ========== build per version, per arch ==========
for %%v in (%VS_LIST%) do (
  call :build_one %%v Win32 || goto label_build_failed
  call :build_one %%v Win64 || goto label_build_failed
)
goto label_build_succeed

:build_one
:: %1 = VS major version (14/15/16/17), %2 = Win32/Win64
setlocal
set "_vnum=%1"
set "_arch=%2"
if "%_vnum%"=="17" set "_vsver=Visual Studio 17 2022"
if "%_vnum%"=="16" set "_vsver=Visual Studio 16 2019"
if "%_vnum%"=="15" set "_vsver=Visual Studio 15 2017"
if "%_vnum%"=="14" set "_vsver=Visual Studio 14 2015"
@echo.
@echo === Building %_arch% with [%_vsver%] (%_build_type%) ===
call .\make_windows.bat %_arch% %_build_type% 1 "%_vsver%"
set "_rc=%ERRORLEVEL%"
endlocal & exit /b %_rc%

:label_build_failed
@echo.
@echo  =========== Error on build, check log above ===========
@echo.
@exit /b 1

:label_build_succeed
@echo.
@echo ... deploy windows(%_build_type%  all detected VS versions) finished ...
@echo.
@exit /b 0
