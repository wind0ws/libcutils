@ECHO OFF &PUSHD %~DP0 &TITLE deploy_android &COLOR 0A

set _build_type=%1
if "%_build_type%" EQU "" (
  set _build_type=Release
) 
@echo BUILD_TYPE=%_build_type%

set _android_stl=%2
if "%_android_stl%" EQU "" (
  set _android_stl=c++_static
) 
@echo ANDROID_STL=%_android_stl%

set BUILD_SCRIPT=make_android.bat
::param 1 for arch
::param 2 for build type
::param 3 for android_stl
call %BUILD_SCRIPT% armeabi-v7a %_build_type% %_android_stl%
IF %ERRORLEVEL% NEQ 0 goto label_build_failed
call %BUILD_SCRIPT% arm64-v8a %_build_type% %_android_stl%
IF %ERRORLEVEL% NEQ 0 goto label_build_failed
call %BUILD_SCRIPT% x86 %_build_type% %_android_stl%
IF %ERRORLEVEL% NEQ 0 goto label_build_failed
call %BUILD_SCRIPT% x86_64 %_build_type% %_android_stl%
IF %ERRORLEVEL% NEQ 0 (goto label_build_failed) else (goto label_build_succeed)

:label_build_failed
@echo.
@echo  =========== Error on build, check log above ===========
@echo.
@exit /b 1


:label_build_succeed
@echo.
@echo ...deploy android(%_build_type%  %_android_stl%) finished...
@echo.
