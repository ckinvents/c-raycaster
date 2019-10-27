#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>

typedef struct _PixBuffer {
    uint32_t* pixels;
    uint32_t width;
    uint32_t height;
    uint32_t color;
} PixBuffer;

void PixBuffer_drawColumn(PixBuffer* buffer, uint32_t x, int32_t y, int32_t h);
void PixBuffer_drawRect(PixBuffer* buffer, SDL_Rect* rect);
void PixBuffer_drawPix(PixBuffer* buffer, uint32_t x, uint32_t y);
void PixBuffer_clearBuffer(PixBuffer* buffer);
void PixBuffer_setColor(PixBuffer* buffer, uint8_t r, uint8_t g, uint8_t b, uint8_t a);

#endif//RENDER_H