@ECHO OFF&PUSHD %~DP0 &TITLE deploy_android &color 0A

::param 1 for arch
::param 2 for build type
call make_android.bat armeabi-v7a Release
call make_android.bat arm64-v8a Release
call make_android.bat x86 Release
call make_android.bat x86_64 Release

@echo.
@echo deploy finished...