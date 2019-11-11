#ifndef RAYENGINE_H
#define RAYENGINE_H

#include <SDL2/SDL.h>
#include "render.h"

typedef struct _Player {
	double x;
	double y;
	double angle;
	double dist;
	double fov;
	double* angleValues;
} Player;

typedef struct _Map {
	unsigned char* data;
	SDL_Color* colorData;
	int numColor;
	int width;
	int height;
	int border;
} Map;

void RayEngine_generateMap(Map* newMap, unsigned char* charList, int width, int height, int border, SDL_Color* colorData, int numColor);
void RayEngine_drawMinimap(PixBuffer* buffer, Player* player, unsigned int width, unsigned int height, Map* map, int blockSize);
void RayEngine_deleteMap(unsigned char** map, int width, int height);
void RayEngine_generateAngleValues(uint32_t width, Player* player);
void RayEngine_raycastRender(PixBuffer* buffer,  Player* player, uint32_t width, uint32_t height, Map* map, double resolution);
void RayEngine_texRaycastRender(PixBuffer* buffer, Player* player, uint32_t width, uint32_t height, Map* map, double resolution, RayTex* texData);
void RayEngine_updatePlayer(Player* player, Map* map, double dt);


#endif//RAYENGINE_H