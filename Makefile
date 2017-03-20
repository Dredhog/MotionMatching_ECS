compiler = clang-5.0

all:
	@$(compiler) -std=c++11 -Wall -Wno-sometimes-uninitialized -Wconversion -Wno-sign-conversion -Wno-missing-braces -Wdouble-promotion *.cpp linear_math/*.cpp -o engine -lGLEW -lGL `sdl2-config --cflags --libs` -lm
	@./engine

