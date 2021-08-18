@ECHO OFF &PUSHD %~DP0 &TITLE deploy_windows &color 0A

::param 1 for arch
::param 2 for build type
::param 3 for WIN_PTHREAD_MODE: 0 use pthread_VC2, 1 use native implement
call make_windows.bat Win32 Debug 0
::call make_windows.bat Win64 Debug 0

@echo.
@echo deploy finished...