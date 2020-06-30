@ECHO OFF&PUSHD %~DP0 &TITLE gen_win32 &color 0A
set cmake_bin=D:\Env\cmake\bin\cmake.exe

set user_choose_win_pthread_mode=%1
if "%user_choose_win_pthread_mode%" EQU "" (
@echo Now you should choose win pthread mode.
goto choose_win_pthread
) else (
@echo your choose pthread_mode is %user_choose_win_pthread_mode%
goto main
)

:choose_win_pthread
@echo "LCU support two mode win pthread mode:"
@echo "  0: use pthreads-win32 lib."
@echo "  1: use default windows native implemention."
@echo .
set /p user_choose_win_pthread_mode="please choose LCU pthread mode:"

:main
rmdir /S /Q build_win32 2>nul
mkdir build_win32 & pushd build_win32
:: VS2019 添加 arch 方式与其他版本不同，默认不加 -A 选项就是Win64(而且不能显式的添加Win64) 
%cmake_bin% -G "Visual Studio 16 2019" -A Win32 .. -DARG_LCU_WIN_PTHREAD_MODE=%user_choose_win_pthread_mode%
::IF %ERRORLEVEL% NEQ 0 %cmake_bin% -G "Visual Studio 15 2017 Win32" .. -DARG_LCU_WIN_PTHREAD_MODE=%user_choose_win_pthread_mode%
::IF %ERRORLEVEL% NEQ 0 %cmake_bin% -G "Visual Studio 14 2015 Win32" .. -DARG_LCU_WIN_PTHREAD_MODE=%user_choose_win_pthread_mode%
popd
::%cmake_bin% --build build_win32 --config Debug

@echo "Generate finished. Press any key to exit..."
@pause>nul
color 0F