# c-raycaster project

![Now with sprites!](image.png)

Connor Ennis, 2019

A basic raycaster engine built in C using SDL2. This is a messy expirement at the moment and more of a learning exercise than anything. I'll be adding more features over time, but for now it has basic functionality and can load simple maps.

Now with sprites: Not fully implemented yet but they render! That's a thing.

WARNING: This project is very much a wip and doesn't even have a makefile yet. I'll be sure to add more info as stuff progresses (including documentation, which is currently very lacking) but this is an expirement so it will never be a particularly good engine. If you want something to use for your own game, with multiple level support and texturing and all of that other jazz (that actually runs well, RIP optimization), there's plenty of other stuff online.

Update: Can, er, finally be compiled. I added the assets that were previously missing so now it should work...
`gcc pixrender.c rayengine.c gameengine.c engine_demo.c -lm -lSDL2 -lSDL2main -O3`
Is recommended for compilation since there isn't a makefile yet... (sorry!)
