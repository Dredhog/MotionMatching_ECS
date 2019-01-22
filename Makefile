compiler = clang++-5.0
warning_flags = -Wall -Wconversion -Wno-missing-braces -Wno-sign-conversion -Wno-writable-strings -Wno-unused-variable  #-Wdouble-promotion 

linker_flags = -lGLEW -lGL `sdl2-config --cflags --libs` -lm -lSDL2_image -lSDL2_ttf


all:
	@$(compiler) $(warning_flags) -g -std=c++11 linux/*.cpp *.cpp linear_math/*.cpp -o engine $(linker_flags)
	@./engine

