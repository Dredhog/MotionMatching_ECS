@echo off

mkdir msvs_build
pushd msvs_build
cl -Zi /std:c++latest /I ..\ ..\win32\*.cpp ..\*.cpp ..\linear_math\*.cpp /Fe: engine glew32.lib opengl32.lib SDL2main.lib SDL2.lib SDL2_image.lib SDL2_ttf.lib /link /SUBSYSTEM:CONSOLE
popd
