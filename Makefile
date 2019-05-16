compiler = clang++-5.0
warning_flags = -Wall -Wconversion -Wno-missing-braces -Wno-sign-conversion -Wno-writable-strings -Wno-unused-variable  #-Wdouble-promotion 

linker_flags = -lGLEW -lGL `sdl2-config --cflags --libs` -lm -lSDL2_image -lSDL2_ttf


all:
	@$(compiler) $(warning_flags) -g -mavx -D USE_DEBUG_PROFILING -std=c++11 linux/*.cpp *.cpp linear_math/*.cpp -o engine $(linker_flags)
	@./engine

plot:
	@python3 plot_measurements.py

models:
	./_build.sh
	./builder/builder ./data/models_actors/conference.obj ./data/built/conference --model --scale 0.0035

animations:
	./clean_anims_actors.sh
	make mixamo
	make cmu

mixamo:
	./build_mixamo.sh

cmu:
	./build_cmu.sh
