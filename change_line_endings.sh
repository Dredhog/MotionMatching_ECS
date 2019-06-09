#for file in *.cpp *.h *.sh *.clang_format *.bat
#for file in .clang-format ./builder/main.cpp ./linear_math/*.h ./linear_math/*.cpp
#for file in .Makefile .gitignore ./linux/*.cpp ./linux/*.h
for file in ./shaders/*.vert ./shaders/*.frag
do 
    vi +':w ++ff=unix' +':q' "$file"
done
