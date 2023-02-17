@ECHO OFF &PUSHD %~DP0 &TITLE deploy_windows &color 0A

::param 1 for arch: "Win32" or "Win64"
::param 2 for build type: "Debug" or "Release"
::param 3 for WIN_PTHREAD_MODE: 0 use pthread_VC2, 1 use native implement
::param 4 for VS_VER: such as "Visual Studio 14 2015"; if you not provide, script auto detect it
:: example: call setup_env.bat Win32 Debug 0 "Visual Studio 14 2015"
call make_windows.bat Win32 Debug 0
::call make_windows.bat Win64 Release 0 "Visual Studio 14 2015"

@echo.
@echo deploy finished... bye...
