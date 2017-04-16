compiler = clang-5.0
warning_flags = -g -Wall -Wconversion -Wno-sign-conversion -Wno-missing-braces -Wdouble-promotion -Wno-writable-strings -Wno-unused-variable
linker_flags = -lGLEW -lGL `sdl2-config --cflags --libs` -lm -lSDL2_image -lSDL2_ttf


all:
	@$(compiler) $(warning_flags) -std=c++11 *.cpp linear_math/*.cpp -o engine $(linker_flags)
	@./engine

