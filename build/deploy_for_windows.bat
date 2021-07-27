@ECHO OFF&PUSHD %~DP0 &TITLE deploy_windows &color 0A

::param 1 for arch
::param 2 for build type
::param 3 for WIN_PTHREAD_MODE: 0 use pthread_VC2, 1 use native implement
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