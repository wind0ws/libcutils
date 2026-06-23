@ECHO OFF
REM libcutils one-click release entry (windows / linux / android / linaro7.5.0)
REM Orchestration logic lives in deploy_release.ps1. This batch is only a thin cmd shell.
REM
REM Usage:
REM   deploy_release.bat                         all platforms, Release, tar.gz package
REM   deploy_release.bat Debug                   change build type
REM   deploy_release.bat Release "windows,linux" only listed platforms (comma separated)
REM
REM Advanced options: call powershell directly, e.g.
REM   powershell -ExecutionPolicy Bypass -File deploy_release.ps1 -Platforms windows -Distro ubuntu_22.04 -NoPackage

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
