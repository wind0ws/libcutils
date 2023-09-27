@ECHO OFF &PUSHD %~DP0 &TITLE cleanup_build_folder &color 0A

set BUILD_FOLDER=.\build\build_*

echo.
echo BUILD_FOLDER=%BUILD_FOLDER%
::rd /s /q build_*
::dir /b /a:d ".\build_*" >nul 2>&1 && echo Folders exist. || echo No folders found.
dir /b /a:d "%BUILD_FOLDER%"

for /d %%G in ("%BUILD_FOLDER%") do rd /s /q "%%~G"

@echo.
@echo "cleanup %BUILD_FOLDER%\build_* folder complete, bye bye..."

ping 127.1 -n 4 >nul