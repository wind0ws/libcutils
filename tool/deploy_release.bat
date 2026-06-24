@ECHO OFF
REM libcutils one-click release entry.
REM Orchestration logic lives in deploy_release.ps1. This batch is only a thin cmd shell.
REM
REM Platforms: windows / android / linux 为内建; 其余任意名只要存在
REM   cmake/toolchains/<name>.toolchain.cmake 即可 (如 linaro7.5.0 / hisi_a7 / r328 ...)，
REM   非 windows/android 一律走 WSL 交叉编译。
REM windows 会探测本机已装的所有 VS 版本(2015/2017/2019/2022)逐个编译，
REM   输出目录带版本号: windows2015_x64 / windows2022_x32 ...
REM
REM Usage:
REM   deploy_release.bat                                   default platforms, Release
REM   deploy_release.bat Debug                             change build type
REM   deploy_release.bat Release "windows,android,linaro7.5.0,hisi_a7"   listed platforms
REM
REM Advanced: call powershell directly, e.g.
REM   powershell -ExecutionPolicy Bypass -File deploy_release.ps1 -Platforms windows,hisi_a7 -Distro ubuntu_22.04 -NoPackage

PUSHD "%~dp0"
TITLE deploy_release

SET "_BUILD_TYPE=%~1"
IF "%_BUILD_TYPE%"=="" SET "_BUILD_TYPE=Release"

SET "_PLATFORMS=%~2"

SET "_PS_ARGS=-BuildType %_BUILD_TYPE%"
IF NOT "%_PLATFORMS%"=="" SET "_PS_ARGS=%_PS_ARGS% -Platforms %_PLATFORMS%"

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0deploy_release.ps1" %_PS_ARGS%
SET "_ERR=%ERRORLEVEL%"

IF "%_ERR%"=="0" (
  ECHO.
  ECHO  =========== release succeeded ===========
  ECHO.
) ELSE (
  ECHO.
  ECHO  =========== release FAILED exit=%_ERR%, check log above ===========
  ECHO.
)

POPD
EXIT /B %_ERR%
