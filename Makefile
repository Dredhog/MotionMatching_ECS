all:
	clang++ -std=c++11 main.cpp -o program `sdl2-config --cflags --libs`
	./program
