compiler = clang-5.0
common_flags = -g -std=c++11 -Wall -Wconversion -Wno-sign-conversion -Wno-missing-braces -Wdouble-promotion -Wno-writable-strings
linker_flags = -L/usr/lib64 -lstdc++ -lm
header_dirs = ../

all:
	@$(compiler) $(common_flags) $(linker_flags) -I $(header_dirs) /usr/lib/x86_64-linux-gnu/libassimp.so main.cpp ../asset.cpp ../linear_math/*.cpp ../linux/linux_file_io.cpp -o builder 
