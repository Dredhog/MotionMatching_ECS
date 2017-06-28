@echo off

mkdir win32_build
pushd win32_build
cl -Zi /std:c++latest ..\*.cpp ..\linear_math\*.cpp /Fe: engine glew32.lib opengl32.lib SDL2main.lib SDL2.lib SDL2_image.lib SDL2_ttf.lib /link /SUBSYSTEM:CONSOLE
popd
