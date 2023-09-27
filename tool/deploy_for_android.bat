@ECHO OFF &PUSHD %~DP0 &TITLE deploy_android &color 0A

set BUILD_SCRIPT=make_android.bat
::param 1 for arch
::param 2 for build type
call %BUILD_SCRIPT% armeabi-v7a Release
IF %ERRORLEVEL% NEQ 0 goto label_build_failed
call %BUILD_SCRIPT% arm64-v8a Release
IF %ERRORLEVEL% NEQ 0 goto label_build_failed
call %BUILD_SCRIPT% x86 Release
IF %ERRORLEVEL% NEQ 0 goto label_build_failed
call %BUILD_SCRIPT% x86_64 Release
IF %ERRORLEVEL% NEQ 0 (goto label_build_failed) else (goto label_build_succeed)

:label_build_failed
@echo.
@echo  ===========Error on build, check log above===========
@echo.
@exit /b 1


:label_build_succeed
@echo.
@echo ...deploy android finished...
@echo.