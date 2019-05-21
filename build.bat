@echo off

IF NOT EXIST msvc_build mkdir msvc_build
pushd msvc_build

del *.pdb > NUL 2> NUL

cl -O3 -nologo -Zi -FC /std:c++latest /I ..\ /I ..\include ..\win32\*.cpp ..\*.cpp ..\linear_math\*.cpp /Fe: engine ..\lib\glew32.lib opengl32.lib ..\lib\SDL2main.lib ..\lib\SDL2.lib ..\lib\SDL2_image.lib ..\lib\SDL2_ttf.lib shcore.lib /link -incremental:no -opt:ref /SUBSYSTEM:CONSOLE

popd
