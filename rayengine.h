#ifndef RAYENGINE_H
#define RAYENGINE_H

#include <SDL2/SDL.h>
#include "pixrender.h"

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

typedef struct _RayColumn {
	RayTex* texture;
	uint32_t texCoord;
	double depth;
	int32_t yCoord;
	uint32_t height;
} RayColumn;

typedef struct _RayBuffer {
	RayColumn layers[255];
	uint8_t numLayers;
} RayBuffer;

typedef struct _RaySprite {
	RayTex* texture;
	double scaleFactor;
	double x;
	double y;
} RaySprite;

void RayEngine_generateMap(Map* newMap, unsigned char* charList, int width, int height, int border, SDL_Color* colorData, int numColor);
void RayEngine_drawMinimap(PixBuffer* buffer, Player* player, unsigned int width, unsigned int height, Map* map, int blockSize);
void RayEngine_deleteMap(unsigned char** map, int width, int height);
void RayEngine_generateAngleValues(uint32_t width, Player* player);
void RayEngine_raySpriteCompute(RayBuffer* rayBuffer, Player* player, uint32_t width, uint32_t height, double resolution, RaySprite* spriteList, uint8_t numSprites);
void RayEngine_raycastCompute(RayBuffer* rayBuffer, Player* player, uint32_t width, uint32_t height, Map* map, double resolution, RayTex* texData);
void RayEngine_raycastRender(PixBuffer* buffer,  Player* player, uint32_t width, uint32_t height, Map* map, double resolution);
void RayEngine_texRaycastRender(PixBuffer* buffer, uint32_t width, uint32_t height, RayBuffer* rayBuffer, double renderDepth);
void RayEngine_updatePlayer(Player* player, Map* map, double dt);


#endif//RAYENGINE_H