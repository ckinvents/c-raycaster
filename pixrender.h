#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>

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

void PixBuffer_drawColumn(PixBuffer* buffer, uint32_t x, int32_t y, int32_t h, SDL_Color color);
void PixBuffer_drawTexColumn(PixBuffer* buffer, uint32_t x, int32_t y, int32_t h, RayTex* texture, uint8_t tileNum, double alphaNum, uint32_t column, double fadePercent, SDL_Color targetColor);
void PixBuffer_drawRect(PixBuffer* buffer, SDL_Rect* rect, SDL_Color color);
void PixBuffer_drawHorizGradient(PixBuffer* buffer, SDL_Rect* rect, SDL_Color colTop, SDL_Color colBottom);
void PixBuffer_drawPix(PixBuffer* buffer, uint32_t x, uint32_t y, SDL_Color color);
void PixBuffer_clearBuffer(PixBuffer* buffer);
void PixBuffer_paletteFilter(PixBuffer* buffer, SDL_Color* palette, int paletteNum);
void PixBuffer_orderDither(PixBuffer* buffer, SDL_Color* palette, int paletteNum, double scaleFactor);
void PixBuffer_orderDither256(PixBuffer* buffer, double scaleFactor);

#endif//RENDER_H