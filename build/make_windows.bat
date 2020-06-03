set cmake_bin=D:\Env\cmake\bin\cmake.exe

mkdir build32 & pushd build32
%cmake_bin% -G "Visual Studio 16 2019" ..
IF %ERRORLEVEL% NEQ 0 %cmake_bin% -G "Visual Studio 15 2017" ..
IF %ERRORLEVEL% NEQ 0 %cmake_bin% -G "Visual Studio 14 2015" ..
popd
%cmake_bin% --build build32 --config Release
mkdir .\output\windows\build32\
cp .\build32\Release\libcutils_test.exe .\output\windows\build32\

echo "compile complete\n Press any key to exit..."
pause