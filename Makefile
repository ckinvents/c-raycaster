##
# Makefile for engine_demo RayEngine demonstration
# Connor Ennis, 2020
##

linux:
	gcc pixrender.c rayengine.c gameengine.c engine_demo.c \
	-lm -lSDL2 -lSDL2main -O3 -o build/linux/engine_demo

linux-debug:
	gcc -Wall pixrender.c rayengine.c gameengine.c engine_demo.c \
	-lm -lSDL2 -lSDL2main -g -o debug/linux/engine_demo

windows:
	x86_64-w64-mingw32-gcc -I../../../win_libs/SDL2/x86_64-w64-mingw32/include \
	pixrender.c rayengine.c gameengine.c engine_demo.c \
	-L../../../win_libs/SDL2/x86_64-w64-mingw32/lib -lm -lmingw32 -lSDL2main -lSDL2 \
	-O3 -m64 -mwindows -o build/windows/engine_demo.exe

textures:
	./img_converter private_assets/world_tex.c assets/world_tex.h

run: build/linux/engine_demo
	./build/linux/engine_demo

run-windows: build/windows/engine_demo.exe
	./build/windows/engine_demo.exe

clean:
	rm build/linux/engine_demo
	rm build/windows/engine_demo.exe