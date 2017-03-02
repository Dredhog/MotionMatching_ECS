compiler = clang-5.0

all:
	@$(compiler) -std=c++11 -Wall -Wconversion -Wdouble-promotion main.cpp -o engine -lGLEW -lGL `sdl2-config --cflags --libs` -lm
	@./engine
