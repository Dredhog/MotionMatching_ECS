compiler = clang-5.0
warning_flags = -Wall -Wconversion -Wno-sign-conversion -Wno-missing-braces -Wdouble-promotion -Wno-writable-strings -Wno-unused-variable
linker_flags = -lGLEW -lGL `sdl2-config --cflags --libs` -lm


all:
	@$(compiler) $(warning_flags) -std=c++11 *.cpp linear_math/*.cpp -o engine $(linker_flags)
	@./engine

