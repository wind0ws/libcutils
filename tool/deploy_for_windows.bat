@ECHO OFF&PUSHD %~DP0 &TITLE deploy_windows &COLOR 0A

set _build_type=%1
if "%_build_type%" EQU "" (
  set _build_type=Release
) 
@echo BUILD_TYPE=%_build_type%

set _vs_ver=%2
@echo VS_VER=%_vs_ver%

::param 1 for arch: "Win32" or "Win64"
::param 2 for build type: "Debug" / "Release" / "MinSizeRel" / "RelWithDebInfo"
::param 3 for WIN_PTHREAD_MODE: 0 use native implement, 1 use pthread_lib, 2 use pthread_dll
::param 4 for VS_VER: such as "Visual Studio 14 2015"; if you not provide, script auto detect it
::  example: call make_windows.bat Win64 Debug 1 "Visual Studio 14 2015 Win64"
::  example: call make_windows.bat Win32 Debug 1 "Visual Studio 14 2015"
::  example: call make_windows.bat Win32 Debug 1 "Visual Studio 16 2019"
::  example: call make_windows.bat Win64 Debug 1 "Visual Studio 16 2019"
::    as you can see, when vs version below 2019, Win64 should append on vs version string.
::    and vs2019/2022 no need to do that. should be careful of this.

call make_windows.bat Win32 %_build_type% 1 %_vs_ver%
IF %ERRORLEVEL% NEQ 0 goto label_build_failed
call make_windows.bat Win64 %_build_type% 1 %_vs_ver%
IF %ERRORLEVEL% NEQ 0 (goto label_build_failed) else (goto label_build_succeed)

:label_build_failed
@echo.
@echo  =========== Error on build, check log above ===========
@echo.
@exit /b 1


:label_build_succeed
@echo.
@echo ... deploy windows(%_build_type%  %_vs_ver%) finished ...
@echo.