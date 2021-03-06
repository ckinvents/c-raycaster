#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>
#define WIDDERSHINS 1024
#define TURNWISE 896

typedef struct _PixBuffer {
    uint32_t* pixels;
    uint32_t width;
    uint32_t height;
} PixBuffer;

typedef struct _RayTex {
	uint32_t* pixData;
	uint32_t tileWidth;
	uint32_t tileHeight;
	uint8_t tileCount;
} RayTex;

PixBuffer* PixBuffer_initPixBuffer(uint32_t width, uint32_t height);
void PixBuffer_delPixBuffer(PixBuffer* buffer);
void PixBuffer_drawColumn(PixBuffer* buffer, uint32_t x, int32_t y, int32_t h, SDL_Color color);
void PixBuffer_drawTexColumn(PixBuffer* buffer, uint32_t x, int32_t y, int32_t h, RayTex* texture, uint8_t tileNum, double alphaNum, uint32_t column, double fadePercent, SDL_Color targetColor);
void PixBuffer_drawRect(PixBuffer* buffer, SDL_Rect* rect, SDL_Color color);
void PixBuffer_drawHorizGradient(PixBuffer* buffer, SDL_Rect* rect, SDL_Color colTop, SDL_Color colBottom);
void PixBuffer_fillBuffer(PixBuffer* target, uint32_t color, double alpha);
void PixBuffer_drawBuffOffset(PixBuffer* target, PixBuffer* source, uint32_t x, uint32_t y, int32_t xOff);
void PixBuffer_clearBuffer(PixBuffer* buffer);
void PixBuffer_paletteFilter(PixBuffer* buffer, SDL_Color* palette, int paletteNum);
void PixBuffer_orderDither(PixBuffer* buffer, SDL_Color* palette, int paletteNum, double scaleFactor);
void PixBuffer_orderDither256(PixBuffer* buffer, double scaleFactor);
void PixBuffer_monochromeFilter(PixBuffer* buffer, SDL_Color targetColor, double fadePercent);
void PixBuffer_inverseFilter(PixBuffer* buffer);
uint32_t PixBuffer_toPixColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SDL_Color PixBuffer_toSDLColor(uint32_t pixColor);
uint32_t PixBuffer_blendAlpha(uint32_t baseColor, uint32_t addColor, double alphaNum);
uint32_t PixBuffer_getPix(PixBuffer* buffer, uint32_t x, uint32_t y);
uint32_t PixBuffer_getTex(RayTex* texture, uint8_t tileNum, uint32_t x, uint32_t y);
void PixBuffer_drawPix(PixBuffer* buffer, uint32_t x, uint32_t y, uint32_t color);
void PixBuffer_drawPixAlpha(PixBuffer* buffer, uint32_t x, uint32_t y, uint32_t color, double alphaNum);
void PixBuffer_drawPixDouble(PixBuffer* buffer, double x, double y, uint32_t color, double alphaNum);

RayTex* RayTex_initFromRGBA(uint8_t* rgbaData, uint32_t tileWidth, uint32_t tileHeight, uint8_t numTiles);
void RayTex_delRayTex(RayTex* tex);

#endif//RENDER_H
