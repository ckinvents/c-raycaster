/**
 * My solution to being unable to
 * apply per-pixel effects using
 * the SDL2 primitive render functions
 * 
 * (See header file for details on
 * PixBuffers)
 */

#include <omp.h>
#include "pixrender.h"

uint32_t getColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

// Precomputed 4x4 bayer matrix
// to be used for ordered dithering
// based on 4 patterns
// (see PixBuffer_orderedDither)
double ditherMatrix[16] = {
    -0.875, 0.125, -0.625, 0.375,
    0.625, -0.375, 0.875, -0.125,
    -0.5, 0.5, -0.75, 0.25,
    1.0, 0.0, 0.75, -0.25
};

/**
 * @brief Draws a column to a pixel buffer
 * Note: drawColumn <b>does not</b> check x bound
 * Ensure that your draw functions choose
 * an X value less than the buffer width
 * @param buffer PixBuffer struct to write to
 * @param x x coordinate of column
 * @param y y coordinate of <b>top</b> of column
 * @param h height of column
 **/
void PixBuffer_drawColumn(PixBuffer* buffer, uint32_t x, int32_t y, int32_t h, SDL_Color color)
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
    #pragma omp parallel for schedule(dynamic,1)
    for (int32_t i = y; i < y + h; i++)
    {
        buffer->pixels[i*buffer->width+x] = getColor(color.r,color.g,color.b,color.a);
    }
}

void PixBuffer_drawTexColumn(PixBuffer* buffer, uint32_t x, int32_t y, int32_t h, RayTex* texture, uint8_t tileNum, double alphaNum, uint32_t column, double fadePercent, SDL_Color targetColor)
{
    if (y + h < 0 || fadePercent > 1.0)
    {
        return;  // Sorry, messy fix but it works
    }
    int32_t offH = h;
    int32_t offY = 0;
    if (y < 0)
    {
        offY = -y;
        h = h + y;
        y = 0;
    }
    if (y + h > buffer->height)
    {
        h = buffer->height - y;
    }
    //#pragma omp parallel for schedule(dynamic,1)
    for (int32_t i = 0; i < h; i++)
    {
        // Calculate pixel to draw from texture
        uint32_t pix = texture->pixData[tileNum*texture->tileWidth*texture->tileHeight + (uint32_t)(((double)(offY + i)/(double)offH)*(texture->tileHeight)) * texture->tileWidth + column];
        int r = (int)(pix >> 3*8);
        int g = (int)((pix >> 2*8) & 0xFF);
        int b = (int)((pix >> 8) & 0xFF);
        int a = (int)(pix & 0xFF);
        if (a)
        {
            int dr = targetColor.r - r;
            int dg = targetColor.g - g;
            int db = targetColor.b - b;
            int da = targetColor.a - a;
            r += (int)((double)dr * fadePercent);
            g += (int)((double)dg * fadePercent);
            b += (int)((double)db * fadePercent);
            a += (int)((double)da * fadePercent);
            if (alphaNum*a != 0 && alphaNum*a != 255) // Alpha transparency, compute alpha based on array colors
            {
                double alpha = ((double)a)/255.0 * (alphaNum);
                uint32_t oldPix = buffer->pixels[(i+y)*buffer->width+x];
                int oldR = (int)(oldPix >> 3*8);
                int oldG = (int)((oldPix >> 2*8) & 0xFF);
                int oldB = (int)((oldPix >> 8) & 0xFF);
                r = (int)((double)r * alpha + (double)oldR * (1-alpha));
                g = (int)((double)g * alpha + (double)oldG * (1-alpha));
                b = (int)((double)b * alpha + (double)oldB * (1-alpha));
            }
            buffer->pixels[(i+y)*buffer->width+x] = getColor(r,g,b,0xff);
        }
    }
}

/**
 * @brief Draws a row to a pixel buffer
 * Note: drawRow <b>does not</b> check x <b>or</b>
 * y bounds. Be careful to ensure x, w, and y
 * parameters are within the buffer size
 * @param buffer PixBuffer struct to write to
 * @param x x coordinate of <b>left</b> of row
 * @param y y coordinate of row
 * @param w width of row
 **/
void PixBuffer_drawRow(PixBuffer* buffer, uint32_t x, uint32_t y, uint32_t w, SDL_Color color)
{
    int r = color.r;
    int g = color.g;
    int b = color.b;
    int a = color.a;
    double alpha = ((double)(color.a))/255.0;
    for (int32_t i = x; i < w; i++)
    {
        if (a) // Alpha transparency, compute alpha based on array colors
        {
            uint32_t oldPix = buffer->pixels[(y*buffer->width)+(i)];
            int oldR = (int)(oldPix >> 3*8);
            int oldG = (int)((oldPix >> 2*8) & 0xFF);
            int oldB = (int)((oldPix >> 8) & 0xFF);
            r = (int)((double)(color.r) * alpha + (double)oldR * (1.0-alpha));
            g = (int)((double)(color.g) * alpha + (double)oldG * (1.0-alpha));
            b = (int)((double)(color.b) * alpha + (double)oldB * (1.0-alpha));
            buffer->pixels[y*buffer->width+i] = getColor(r,g,b,0xff);
        }
    }
}

/**
 * @brief Draws a filled rectangle to a pixel buffer
 * Note: drawRect does check bounds, but for
 * the sake of speed it is recommended
 * that rectangles are constrained to
 * buffer size to avoid wasted cycles
 * @param buffer PixBuffer struct to write to
 * @param rect SDL_Rect struct with coordinate and dimension data
 **/
void PixBuffer_drawRect(PixBuffer* buffer, SDL_Rect* rect, SDL_Color color)
{
    if (rect->x < buffer->width)
    {
        for (uint32_t i = rect->x; i < rect->x + rect->w; i++)
        {
            if (i < buffer->width)
            {
                PixBuffer_drawColumn(buffer, i, rect->y, rect->h, color);
            }
        }
    }
}

void PixBuffer_drawHorizGradient(PixBuffer* buffer, SDL_Rect* rect, SDL_Color colTop, SDL_Color colBottom)
{
    if (rect->x < buffer->width && rect->x+rect->w <= buffer->width)
    {
        double rStep = ((double)colBottom.r - (double)colTop.r) / rect->h;
        double gStep = ((double)colBottom.g - (double)colTop.g) / rect->h;
        double bStep = ((double)colBottom.b - (double)colTop.b) / rect->h;
        double aStep = ((double)colBottom.a - (double)colTop.a) / rect->h;
        SDL_Color drawColor;
        //#pragma omp parallel for schedule(dynamic,1) private(drawColor)
        for (uint32_t i = 0; i < rect->h; i++)
        {
            if (i < buffer->height)
            {
                drawColor.r = colTop.r+(int)(rStep*i);
                drawColor.g = colTop.g+(int)(gStep*i);
                drawColor.b = colTop.b+(int)(bStep*i);
                drawColor.a = colTop.a+(int)(aStep*i);
                PixBuffer_drawRow(buffer, rect->x, rect->y+i, rect->w, drawColor);
            }
        }
    }
}

/**
 * @brief Clears buffer array to 0x00
 * Useful if you need to quickly reuse a buffer
 * for drawing layers/graphics updates. Sets to
 * transparent black using memset
 * @param buffer PixBuffer struct to clear
 **/
void PixBuffer_clearBuffer(PixBuffer* buffer)
{
    memset(buffer->pixels, 0, buffer->width * buffer->height * 4);
}

/**
 * @brief Remaps RGB buffer colors to a given pallette
 * This filter uses RGB color distance to match every pixel
 * color to the closest color in the palette array. Each pixel
 * is then updated with this new color, effectively palettizing
 * the buffer.
 * Note: it is important to ensure paletteNum is no longer than
 * the palette list, otherwise your this will index nonexistant colors
 * and make your output look really funky. And possibly segfault I guess
 * @param buffer PixBuffer to palettize
 * @param palette SDL_Color array to quantitize to
 * @param paletteNum length of color palette
 * @todo consolidate palletteFilter and nearestColor functions
 **/
void PixBuffer_paletteFilter(PixBuffer* buffer, SDL_Color* palette, int paletteNum)
{
    int r;
    int g;
    int b;
	int colNum = 0;
    for (uint32_t p = 0; p < buffer->width * buffer->height; p++)
    {
        if (buffer->pixels[p] != 0)
        {
            r = (int)(buffer->pixels[p] >> 3*8);
            g = (int)((buffer->pixels[p] >> 2*8) & 0xFF);
            b = (int)((buffer->pixels[p] >> 8) & 0xFF);
	        uint32_t minColorDif = 0xFF*0xFF*3;//adjustedColorDiff(r, g, b, colorPallette[0].r, colorPallette[0].g, colorPallette[0].b);
	        for (int i = 0; i < paletteNum; i++)
	        {
		        uint32_t colorDif = (uint32_t)(palette[i].r - r)*(palette[i].r - r) + (uint32_t)(palette[i].g - g)*(palette[i].g - g) + (uint32_t)(palette[i].b - b)*(palette[i].b - b);
		        //double colorDif = adjustedColorDiff(r, g, b, colorPallette[i].r, colorPallette[i].g, colorPallette[i].b);//(uint32_t)(rFact * (double)(colorPallette[i].r - r))*(rFact * (double)(colorPallette[i].r - r)) + (gFact * (double)(colorPallette[i].g - g))*(gFact * (double)(colorPallette[i].g - g)) + (bFact * (double)(colorPallette[i].b - b))*(bFact * (double)(colorPallette[i].b - b));
		        if (colorDif < minColorDif)
		        {
			        minColorDif = colorDif;
			        colNum = i;
		        }
	        }
	        buffer->pixels[p] = (uint32_t)(palette[colNum].r) << 3*8 | (uint32_t)(palette[colNum].g) << 2*8 | (uint32_t)(palette[colNum].b) << 8 | (uint32_t)0xFF;
        }
    }
}

/**
 * @brief Internal function called by orderedDither for quantitization
 * @param palette SDL_Color array to quantitize to
 * @param paletteNum length of color palette
 * @param colorDat buffer format color to quantititize
 * @return buffer format color of closest palette match
 **/
uint32_t getNearestColor(SDL_Color* palette, int paletteNum, uint32_t colorDat)
{
    int r = (int)(colorDat >> 3*8);
    int g = (int)((colorDat >> 2*8) & 0xFF);
    int b = (int)((colorDat >> 8) & 0xFF);
    int colNum = 0;
    uint32_t minColorDif = 0xFF*0xFF*3;//adjustedColorDiff(r, g, b, colorPallette[0].r, colorPallette[0].g, colorPallette[0].b);
    for (int i = 0; i < paletteNum; i++)
    {
        uint32_t colorDif = (uint32_t)(palette[i].r - r)*(palette[i].r - r) + (uint32_t)(palette[i].g - g)*(palette[i].g - g) + (uint32_t)(palette[i].b - b)*(palette[i].b - b);
        if (colorDif < minColorDif)
        {
            minColorDif = colorDif;
            colNum = i;
        }
    }
    return (uint32_t)(palette[colNum].r) << 3*8 | (uint32_t)(palette[colNum].g) << 2*8 | (uint32_t)(palette[colNum].b) << 8 | (uint32_t)0xFF;
}

/**
 * @brief Applies an ordered dither effect to a buffer
 * The algorithm for this is somewhat based on the pseudocode
 * from the wikipedia page, but adapted once I found out
 * how this works
 * Each pixel color in the buffer is added to a corresponding
 * matrix value (which repeats across the buffer dimension)
 * The weights are arranged based on the pattern of dithering
 * (which pixels should be lightened or darkened first based on
 * change in brightness) with 16 possible levels (for a 4x4 as
 * provided). The scaleFactor represents the magnitude to which
 * the weight shall brighten or darken the color for pallette
 * assignment, which shall increase the dither color range while
 * also compromising on color accuracy (since the palletes given
 * are not dispersed, this should be adjusted to taste). The brightened
 * or darkened color (again, magnitude of the weight determining which
 * pixels shall change the most and, thus, for a solid color region, which
 * pixels shall change first) is then processed by the getNearestColor
 * function which will then assign the pixel to a color brighter, darker
 * or equal to the otherwise undithered quantitization based on the
 * weight values.
 * @param buffer PixBuffer to dither
 * @param palette SDL_Color array to quantitize to
 * @param paletteNum length of color palette
 * @param scaleFactor intensity of dither weights
 **/
void PixBuffer_orderDither(PixBuffer* buffer, SDL_Color* palette, int paletteNum, double scaleFactor)
{
    // Components to decode RGBA format
    int32_t r;
    int32_t g;
    int32_t b;
    int32_t dithFactor;
    // default: 4
    // How much the matrix weights should vary the input colors
    int32_t newColor;
    //#pragma omp parallel for schedule(dynamic,1) private(r,g,b,dithFactor,newColor)
    for (uint32_t y = 0; y < buffer->height; y++)
    {
        for (uint32_t x = 0; x < buffer->width; x++)
        {
            if (buffer->pixels[y*buffer->width+x] != 0)
            {
                r = (int)(buffer->pixels[y*buffer->width+x] >> 3*8);
                g = (int)((buffer->pixels[y*buffer->width+x] >> 2*8) & 0xFF);
                b = (int)((buffer->pixels[y*buffer->width+x] >> 8) & 0xFF);
                // Finds associated dither weight, which will
                // be applied to the color to bring it above or below the threshold
                // for getNearestColor to assign a varied brightness
                dithFactor = scaleFactor*ditherMatrix[(y%4)*4+(x%4)];
                r = (int)(r + dithFactor);
                if (r > 255)
                {
                    r = 255;
                }
                else if (r < 0)
                {
                    r = 0;
                }
                g = (int)(g + dithFactor);
                if (g > 255)
                {
                    g = 255;
                }
                else if (g < 0)
                {
                    g = 0;
                }
                b = (int)(b + dithFactor);
                if (b > 255)
                {
                    b = 255;
                }
                else if (b < 0)
                {
                    b = 0;
                }
                newColor = (uint32_t)(r) << 3*8 | (uint32_t)(g) << 2*8 | (uint32_t)(b) << 8 | (uint32_t)0xFF;
                buffer->pixels[y*buffer->width+x] = getNearestColor(palette, paletteNum, newColor);
            }
        }
    }
}

uint32_t to8BitColor(uint32_t colorDat)
{
    int r = (int)(colorDat >> 3*8);
    int g = (int)((colorDat >> 2*8) & 0xFF);
    int b = (int)((colorDat >> 8) & 0xFF);
    double divFact = 255.0/7;
    int colNum = 0;
    int newR = (int)ceil(round((double)r / 255.0*15) * (255.0/15));
    int newG = (int)ceil(round((double)g / 255.0*15) * (255.0/15));
    int newB = (int)ceil(round((double)b / 255.0*15) * (255.0/15));
    return (uint32_t)(newR) << 3*8 | (uint32_t)(newG) << 2*8 | (uint32_t)newB << 8 | (uint32_t)0xFF;
}

void PixBuffer_orderDither256(PixBuffer* buffer, double scaleFactor)
{
    // Components to decode RGBA format
    int32_t r;
    int32_t g;
    int32_t b;
    int32_t dithFactor;
    // default: 4
    // How much the matrix weights should vary the input colors
    int32_t newColor;

    for (uint32_t y = 0; y < buffer->height; y++)
    {
        for (uint32_t x = 0; x < buffer->width; x++)
        {
            if (buffer->pixels[y*buffer->width+x] != 0)
            {
                r = (int)(buffer->pixels[y*buffer->width+x] >> 3*8);
                g = (int)((buffer->pixels[y*buffer->width+x] >> 2*8) & 0xFF);
                b = (int)((buffer->pixels[y*buffer->width+x] >> 8) & 0xFF);
                // Finds associated dither weight, which will
                // be applied to the color to bring it above or below the threshold
                // for getNearestColor to assign a varied brightness
                dithFactor = scaleFactor*ditherMatrix[(y%4)*4+(x%4)];
                r = (int)(r + dithFactor);
                if (r > 255)
                {
                    r = 255;
                }
                else if (r < 0)
                {
                    r = 0;
                }
                g = (int)(g + dithFactor);
                if (g > 255)
                {
                    g = 255;
                }
                else if (g < 0)
                {
                    g = 0;
                }
                b = (int)(b + dithFactor);
                if (b > 255)
                {
                    b = 255;
                }
                else if (b < 0)
                {
                    b = 0;
                }
                newColor = (uint32_t)(r) << 3*8 | (uint32_t)(g) << 2*8 | (uint32_t)(b) << 8 | (uint32_t)0xFF;
                buffer->pixels[y*buffer->width+x] = to8BitColor(newColor);
            }
        }
    }
}

/**
 * @brief Returns color formatted to RGBA format
 * @param buffer PixBuffer to set color for
 * @param r SDL_Color red component
 * @param g SDL_Color green component
 * @param b SDL_Color blue component
 * @param a SDL_Color alpha component
 **/
uint32_t getColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return ((uint32_t)r << 3*8 | (uint32_t)g << 2*8 | (uint32_t)b << 8 | (uint32_t)a);
}

/**
 * @brief Draws a single pixel to the PixBuffer
 * @param buffer PixBuffer to draw to
 * @param x x coordinate of pixel
 * @param y y coordinate of pixel
 **/
void PixBuffer_drawPix(PixBuffer* buffer, uint32_t x, uint32_t y, SDL_Color color)
{
    if (x < buffer->width && y < buffer->height)
    {
        buffer->pixels[y*buffer->width+x] = getColor(color.r,color.g,color.g,color.a);
    }
}