CFLAGS = -std=c++17 #-Wall# -O2
LDFLAGS = -lSDL3
#LIBRARIES = -IC:/SDL3/SDL_mingw_3.4.0/include/ -LC:/SDL3/SDL_mingw_3.4.0/lib/
LIBRARIES = -IC:/SDL3/SDL_mingw_3.4.4/SDL3-3.4.4/i686-w64-mingw32/include -LC:/SDL3/SDL_mingw_3.4.4/SDL3-3.4.4/i686-w64-mingw32/lib
FILE_OUT = "fallingSand.exe"
FILES_IN = main.cpp src/*.cpp #src/*/*/*.cpp

compile_test: compile_shaders test

test: compile run

compile: main.cpp
	g++ $(CFLAGS) $(LIBRARIES) $(FILES_IN) $(LDFLAGS) -o $(FILE_OUT)

run:
	./$(FILE_OUT)

clean:
	rm -f $(FILE_OUT)

debug:
	g++ $(CFLAGS) -g $(LIBRARIES) $(FILES_IN) $(LDFLAGS) -o $(FILE_OUT)
	gdb $(FILE_OUT)

compile_shaders:
	cd shaders/source; sh.exe compile.sh

clean_shaders:
	rm shaders/compiled/DXIL/*
	rm shaders/compiled/MSL/*
	rm shaders/compiled/SPIRV/*