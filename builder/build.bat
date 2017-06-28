@echo off

mkdir assimp_build
pushd assimp_build
cl /std:c++latest /I ..\..\ ..\main.cpp ..\..\asset.cpp /Fe: builder assimp.lib
popd
