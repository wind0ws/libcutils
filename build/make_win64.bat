@ECHO OFF&PUSHD %~DP0 &TITLE make_win64 &color 0A
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
@echo "  1: use windows native implemention."
@echo .
set /p user_choose_win_pthread_mode="please choose LCU pthread mode:"

:main
set output_dir=.\\output\\windows\\win64
rmdir /S /Q build_win64 2>nul
mkdir build_win64 & pushd build_win64
:: VS2019 添加 arch 方式与其他版本不同，默认不加 -A 选项就是Win64(而且不能显式的添加Win64)
%cmake_bin% -G "Visual Studio 16 2019" .. -DCMAKE_BUILD_TYPE=Release -DARG_LCU_WIN_PTHREAD_MODE=%user_choose_win_pthread_mode% -DARG_LCU_OUTPUT_DIR=%output_dir% 
IF %ERRORLEVEL% NEQ 0 %cmake_bin% -G "Visual Studio 15 2017 Win64" .. -DARG_LCU_WIN_PTHREAD_MODE=%user_choose_win_pthread_mode% -DARG_LCU_OUTPUT_DIR=%output_dir% 
IF %ERRORLEVEL% NEQ 0 %cmake_bin% -G "Visual Studio 14 2015 Win64" .. -DARG_LCU_WIN_PTHREAD_MODE=%user_choose_win_pthread_mode% -DARG_LCU_OUTPUT_DIR=%output_dir% 
popd
%cmake_bin% --build build_win64 --config Release
mkdir %output_dir%
copy /Y .\\build_win64\\Release\\* %output_dir%\\

@echo.
@echo "compile complete. Press any key to exit..."
@pause>nul
color 0F