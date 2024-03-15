@ECHO OFF & PUSHD %~DP0 & TITLE setup_env & COLOR 0A

::for /f %%a in ('dir /a:d /b %ANDROID_SDK%\cmake\') do set cmake_version=%%a
::echo "find cmake version %cmake_version%"
::set cmake_version=3.10.2.4988404
::set CMAKE_BIN=%ANDROID_SDK%\cmake\%cmake_version%\bin\cmake.exe
::set NINJA_BIN=%ANDROID_SDK%\cmake\%cmake_version%\bin\ninja.exe
set CMAKE_BIN=cmake.exe
@echo cmake version: 
%CMAKE_BIN% --version
set ERR_CODE=%ERRORLEVEL%
IF %ERR_CODE% NEQ 0 (
   set CMAKE_BIN=D:\env\cmake\bin\cmake.exe
   @echo.
   @echo "cmake not in your environment, we point a default location for you" 
   if not exist %CMAKE_BIN% (
     @echo ERROR: %CMAKE_BIN% not exists!!
     @exit /b 2
   )
)
@echo.
set NINJA_BIN=ninja.exe
@echo ninja version: 
%NINJA_BIN% --version
set ERR_CODE=%ERRORLEVEL%
IF %ERR_CODE% NEQ 0 (
   set NINJA_BIN=D:\env\cmake\bin\ninja.exe
   @echo.
   @echo "ninja not in your environment, we point a location for you" 
   if not exist %NINJA_BIN% (
     @echo ERROR: %NINJA_BIN% not exists!!
     @exit /b 2
   )
)
@echo.

@echo.
@echo =================== Your Environment ===================
@echo CMAKE_BIN=%CMAKE_BIN%
@echo NINJA_BIN=%NINJA_BIN%
@echo ========================================================
@echo.


set BUILD_ABI=%1
if "%BUILD_ABI%" EQU "" (
  @echo Now you should input build abi.
  goto label_input_abi
) else (
  @echo your BUILD_ABI: %BUILD_ABI%
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
if "%BUILD_TYPE%" EQU "MinSizeRel" goto label_run_next_bat
if "%BUILD_TYPE%" EQU "RelWithDebInfo" goto label_run_next_bat
@echo unknown BUILD_TYPE=%BUILD_TYPE%, available types are: "Debug" / "Release" / "MinSizeRel" / "RelWithDebInfo"
@exit /b 2

:label_run_next_bat
@echo.
@echo   hello... now compile it(%BUILD_ABI%  %BUILD_TYPE%)...
@echo.
@exit /b 0
