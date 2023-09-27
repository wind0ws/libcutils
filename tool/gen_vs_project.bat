@ECHO OFF &PUSHD %~DP0 &TITLE gen_win32 &color 0A

::param 1 for arch: "Win32" or "Win64"
::param 2 for build type: "Debug" or "Release"
::param 3 for WIN_PTHREAD_MODE: 0 use pthread_VC2, 1 use native implement
::param 4 for VS_VER: such as "Visual Studio 14 2015"; if you not provide, script auto detect the newest version of vs
::   example: call setup_env.bat Win32 Debug 0 "Visual Studio 14 2015"

call make_windows.bat Win32 Debug 0
::call make_windows.bat Win64 Release 0 "Visual Studio 14 2015 Win64"
::call make_windows.bat Win32 Release 0 "Visual Studio 14 2015"
::call make_windows.bat Win64 Release 0 "Visual Studio 15 2017"
::call make_windows.bat Win32 Release 0 "Visual Studio 15 2017"
::  as you can see, vs version below 2017, Win64 should append on version string.
::  vs2017/2019/2022 no need do that. should be careful of this

@echo.
@echo deploy finished... bye...
