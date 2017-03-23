compiler = clang-5.0
common_flags = -O3 -std=c++11 -Wall -Wconversion -Wno-sign-conversion -Wno-missing-braces -Wdouble-promotion -Wno-writable-strings
linker_flags = -L/usr/lib64 -lstdc++
header_dirs = ../


all:
	@$(compiler) $(common_flags) $(linker_flags) -I $(header_dirs) /usr/lib/x86_64-linux-gnu/libassimp.so main.cpp -o builder 
