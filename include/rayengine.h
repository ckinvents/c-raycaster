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

enum BufferLayer {
	BL_BASE,
	BL_ALPHA
};

typedef struct {
	PixBuffer* pixelBuffer;
	PixBuffer* alphaBuffer;
	double* pixelDepth;
	double* alphaDepth;
} DepthBuffer;

typedef struct _RaySprite {
	RayTex* texture;
	uint8_t frameNum;
	double alphaNum;
	double scaleFactor;
	double x;
	double y;
	double h;
} RaySprite;

DepthBuffer* RayEngine_initDepthBuffer(uint32_t width, uint32_t height);
double RayEngine_getDepth(DepthBuffer* buffer, uint32_t x, uint32_t y, uint8_t layer);
void RayEngine_setDepth(DepthBuffer* buffer, uint32_t x, uint32_t y, uint8_t layer, double depth);
void RayEngine_drawPix(DepthBuffer* buffer, uint32_t x, uint32_t y, uint32_t color, double alphaNum, double depth);
void RayEngine_drawTexColumn(DepthBuffer* buffer, uint32_t x, int32_t y, int32_t h, double depth, RayTex* texture, uint8_t tileNum, double alphaNum, uint32_t column, double fadePercent, SDL_Color targetColor);
void RayEngine_renderBuffer(DepthBuffer* buffer);
void RayEngine_resetDepthBuffer(DepthBuffer* buffer);
void RayEngine_delDepthBuffer(DepthBuffer* buffer);
void RayEngine_generateMap(Map* newMap, unsigned char* charList, int width, int height, int border, SDL_Color* colorData, int numColor);
void RayEngine_initSprite(RaySprite* newSprite, RayTex* texture, double scaleFactor, double alphaNum, double x, double y, double h);
void RayEngine_draw2DSprite(PixBuffer* buffer, RaySprite sprite, double angle);
void RayEngine_generateAngleValues(uint32_t width, Camera* camera);
void RayEngine_draw3DSprite(DepthBuffer* rayBuffer, Camera* camera, uint32_t width, uint32_t height, double resolution, RaySprite sprite);
void RayEngine_raycastRender(DepthBuffer* rayBuffer, Camera* camera, uint32_t width, uint32_t height, Map* map, double resolution, RayTex* texData);
void RayEngine_texRenderFloor(PixBuffer* buffer, Camera* camera, uint32_t width, uint32_t height, Map* groundMap, double resolution, RayTex* texData, uint8_t tileNum);
void RayEngine_texRenderCeiling(PixBuffer* buffer, Camera* camera, uint32_t width, uint32_t height, Map* ceilingMap, RayTex* texData, uint8_t tileNum);


#endif//RAYENGINE_H