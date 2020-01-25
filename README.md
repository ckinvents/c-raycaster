# c-raycaster project

![Now with sprites!](image.png)

Connor Ennis, 2019-2020

A basic raycaster engine built in C using SDL2. This is a messy expirement at the moment and more of a learning exercise than anything. I'll be adding more features over time, but for now it has basic functionality and can load simple maps.

Now with sprites: Not fully implemented yet but they render! That's a thing.

WARNING: This project is very much a wip and doesn't even have a makefile yet. I'll be sure to add more info as stuff progresses (including documentation, which is currently very lacking) but this is an expirement so it will never be a particularly good engine. If you want something to use for your own game, with multiple level support and texturing and all of that other jazz (that actually runs well, RIP optimization), there's plenty of other stuff online.

Update: Can, er, finally be compiled. I added the assets that were previously missing so now it should work...
`gcc pixrender.c rayengine.c gameengine.c engine_demo.c -lm -lSDL2 -lSDL2main -O3`
Is recommended for compilation since there isn't a makefile yet... (sorry!)

Controls:
- w, s to move forward and backwards
- q, e to strafe left and right, respectively
- a, d to rotate counterclockwise and clockwise (respectively, duh. I'm not a masochist \[hmmm... well...\])
- shift to sprint in literally every direction (minus turning, personal preference)

Current features:
- Textured grid-based raycasting on a single plane
- Pixel scaling, dithering, and palletization
- Automatic texture scaling
- Gradient and rectangle drawing, as well as arbitrary pixel drawing functions
- 3D sprites with depth buffering, alpha transparency, animation, height and scaling
- 2D overlay sprites with alpha transparency and scaling
- Depth fog (sort of)
- Alpha translucency and multi-collision ray tracing with supported textures
- Adjustable player controls and maps
- Map collisions

Todo:
- AI and collisions for entities
- More complex sprite handling
- Improved memory management
- Floors & ceilings (yikes)
- Scrolling skyboxes
- Multi-level maps with transitions/procedural generation
- Player height
- More pixel shaders
- Full overlays
- Text engine
- Audio engine
- Single-plane offset walls
- Animated wall textures
- Interactive objects
- Additional, better reference artwork (I'm doing my best, it takes practice)
- Controller support (yes, UDXS, including the Steam controller)
- Player information (inventory, health, etc)
- Maybe make an actual game with this thing???
- *Actual documentation* D:
- Oh yeah, probably a state system for events. Might be nice.
