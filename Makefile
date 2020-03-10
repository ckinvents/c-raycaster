##
# Makefile for engine_demo RayEngine demonstration
# Connor Ennis, 2020
##

build:
	gcc pixrender.c rayengine.c gameengine.c engine_demo.c \
	-lm -lSDL2 -lSDL2main -O3 -o engine_demo

textures:
	./img_converter private_assets/world_tex.c assets/world_tex.h

run: engine_demo
	./engine_demo

clean:
	rm engine_demo