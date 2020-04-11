#ifndef RAYENGINE_H
#define RAYENGINE_H

#include "pixrender.h"

typedef struct _Camera {
	double x;
	double y;
	double h;
	double angle;
	double dist;
	double fov;
	double angleValues[WIDDERSHINS];
} Camera;

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
	uint8_t texCoord;
	uint8_t tileNum;
	double alphaNum;
	double depth;
	int32_t yCoord;
	uint32_t height;
} RayColumn;

typedef struct _RayBuffer {
	RayColumn layers[32];
	uint8_t numLayers;
} RayBuffer;

typedef struct _RaySprite {
	RayTex* texture;
	uint8_t frameNum;
	double alphaNum;
	double scaleFactor;
	double x;
	double y;
	double h;
} RaySprite;

void RayEngine_generateMap(Map* newMap, unsigned char* charList, int width, int height, int border, SDL_Color* colorData, int numColor);
void RayEngine_initSprite(RaySprite* newSprite, RayTex* texture, double scaleFactor, double alphaNum, double x, double y, double h);
void RayEngine_draw2DSprite(PixBuffer* buffer, RaySprite sprite);
void RayEngine_generateAngleValues(uint32_t width, Camera* camera);
void RayEngine_raySpriteCompute(RayBuffer* rayBuffer, Camera* camera, uint32_t width, uint32_t height, double resolution, RaySprite sprite);
void RayEngine_raycastCompute(RayBuffer* rayBuffer, Camera* camera, uint32_t width, uint32_t height, Map* map, double resolution, RayTex* texData);
void RayEngine_raycastRender(PixBuffer* buffer,  Camera* camera, uint32_t width, uint32_t height, Map* map, double resolution);
void RayEngine_texRaycastRender(PixBuffer* buffer, uint32_t width, uint32_t height, RayBuffer* rayBuffer, double renderDepth);
void RayEngine_texRenderFloor(PixBuffer* buffer, Camera* camera, uint32_t width, uint32_t height, Map* groundMap, double resolution, RayTex* texData, uint8_t tileNum);
void RayEngine_texRenderCeiling(PixBuffer* buffer, Camera* camera, uint32_t width, uint32_t height, Map* ceilingMap, RayTex* texData, uint8_t tileNum);


#endif//RAYENGINE_H