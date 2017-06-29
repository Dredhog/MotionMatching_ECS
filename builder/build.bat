@echo off

mkdir assimp_build
pushd assimp_build
cl /std:c++latest /I ..\..\ /I ..\..\include /I ..\..\win32 ..\main.cpp ..\..\asset.cpp /Fe: builder ..\..\lib\assimp.lib
popd
