set cmake_bin=D:\Env\cmake\bin\cmake.exe

mkdir build64 & pushd build64
:: VS2019 添加 arch 方式与其他版本不同，默认不加 -A 选项就是Win64(而且不能显式的添加Win64)
%cmake_bin% -G "Visual Studio 16 2019" ..
IF %ERRORLEVEL% NEQ 0 %cmake_bin% -G "Visual Studio 15 2017 Win64" ..
IF %ERRORLEVEL% NEQ 0 %cmake_bin% -G "Visual Studio 14 2015 Win64" ..
popd
%cmake_bin% --build build64 --config Release
mkdir .\output\windows\build64\
cp .\build64\Release\libcutils_test.exe .\output\windows\build64\
cp .\build64\Release\lcu.lib .\output\windows\build64\
cp .\build64\Release\lcu.dll .\output\windows\build64\

echo "compile complete\n Press any key to exit..."
pause