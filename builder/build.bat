@echo off

mkdir assimp_build
pushd assimp_build
cl /std:c++latest /EHsc  /I ..\..\ /I ..\..\include /I ..\..\win32  ..\..\win32\win32_file*.cpp ..\main.cpp ..\..\asset.cpp ..\..\linear_math\*.cpp /Fe: builder ..\..\lib\assimp.lib
popd
