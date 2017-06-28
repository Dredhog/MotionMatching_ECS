@echo off

mkdir msvs_build
pushd msvs_build
cl -Zi /DWIN32_DEBUG=0 /std:c++latest /I ..\ /I ..\include ..\win32\*.cpp ..\*.cpp ..\linear_math\*.cpp /Fe: engine ..\lib\glew32.lib opengl32.lib ..\lib\SDL2main.lib ..\lib\SDL2.lib ..\lib\SDL2_image.lib ..\lib\SDL2_ttf.lib /link /SUBSYSTEM:CONSOLE
popd
