@ECHO OFF &PUSHD %~DP0 &TITLE cleanup_build_folder &color 0A

::rd /s /q build_*
::dir /b /a:d ".\build_*" >nul 2>&1 && echo Folders exist. || echo No folders found.
dir /b /a:d ".\build_*"

for /d %%G in (".\build_*") do rd /s /q "%%~G"

@echo.
@echo "cleanup build_* folder complete, bye bye..."

ping 127.1 -n 4 >nul