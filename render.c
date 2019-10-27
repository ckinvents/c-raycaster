/**
 * Kill me now
 * My solution to being unable to
 * apply per-pixel effects using
 * the SDL2 primitive render functions
 * TBH this is based on some thing
 * I found online so whoopdie doo hope
 * I understand it well
 * Edit: didn't even need the thing
 * online, textures are a lot more
 * straightforward than I thought
 */

#include "render.h"


void PixBuffer_drawColumn(PixBuffer* buffer, uint32_t x, int32_t y, int32_t h)
{
    if (y < 0)
    {
        h = h + y;
        y = 0;
    }
    if (y + h > buffer->height)
    {
        h = buffer->height - y;
    }
    for (int32_t i = y; i < y + h; i++)
    {
        buffer->pixels[i*buffer->width+x] = buffer->color;
    }
}

void PixBuffer_drawRect(PixBuffer* buffer, SDL_Rect* rect)
{
    if (rect->x < buffer->width)
    {
        for (uint32_t i = rect->x; i < rect->x + rect->w; i++)
        {
            if (i < buffer->width)
            {
                PixBuffer_drawColumn(buffer, i, rect->y, rect->h);
            }
        }
    }
}

void PixBuffer_clearBuffer(PixBuffer* buffer)
{
    memset(buffer->pixels, 0, buffer->width * buffer->height * 4);
}

void PixBuffer_setColor(PixBuffer* buffer, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    buffer->color = (uint32_t)r << 3*8 | (uint32_t)g << 2*8 | (uint32_t)b << 8 | (uint32_t)a;
}

void PixBuffer_drawPix(PixBuffer* buffer, uint32_t x, uint32_t y)
{
    if (x < buffer->width && y < buffer->height)
    {
        buffer->pixels[y*buffer->width+x] = buffer->color;
    }
}