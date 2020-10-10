##
# Makefile for engine_demo RayEngine demonstration
# Connor Ennis, 2020
##

# Windows SDL2 Library directory
DIR_WINLIBS = ./../../winlibs

linux: builddir
	gcc pixrender.c rayengine.c gameengine.c engine_demo.c \
	-lm -lSDL2 -lSDL2main -O3 -o build/linux/engine_demo

linux-debug: debugdir
	gcc -Wall pixrender.c rayengine.c gameengine.c engine_demo.c \
	-lm -lSDL2 -lSDL2main -g -o debug/linux/engine_demo

windows: builddir
	x86_64-w64-mingw32-gcc -I$(DIR_WINLIBS)/SDL2/x86_64-w64-mingw32/include \
	pixrender.c rayengine.c gameengine.c engine_demo.c \
	-L$(DIR_WINLIBS)/SDL2/x86_64-w64-mingw32/lib -lm -lmingw32 -lSDL2main -lSDL2 \
	-O3 -m64 -mwindows -o build/windows/engine_demo.exe

builddir:
	mkdir -p build/linux
	mkdir -p build/windows

debugdir:
	mkdir -p debug/linux

textures:
	./img_converter private_assets/world_tex.c assets/world_tex.h

run-linux: linux
	./build/linux/engine_demo

run-windows: windows
	./build/windows/engine_demo.exe

clean:
	rm build/linux/engine_demo
	rm build/windows/engine_demo.exe
