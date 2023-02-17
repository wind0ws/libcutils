@ECHO OFF&PUSHD %~DP0 &TITLE deploy_windows &color 0A

::param 1 for arch: "Win32" or "Win64"
::param 2 for build type: "Debug" or "Release"
::param 3 for WIN_PTHREAD_MODE: 0 use pthread_VC2, 1 use native implement
::param 4 for VS_VER: such as "Visual Studio 14 2015"; if you not provide, script auto detect it
:: example: call setup_env.bat Win32 Debug 0 "Visual Studio 14 2015"
call make_windows.bat Win32 Release 0
IF %ERRORLEVEL% NEQ 0 goto label_build_failed
call make_windows.bat Win64 Release 0
IF %ERRORLEVEL% NEQ 0 (goto label_build_failed) else (goto label_build_succeed)

:label_build_failed
@echo.
@echo  ===========Error on build, check log above===========
@echo.
@exit /b 1


:label_build_succeed
@echo.
@echo ...deploy windows finished...
@echo.