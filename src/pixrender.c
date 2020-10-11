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

/**
 * Precomputed 4x4 bayer matrix
 * to be used for ordered dithering
 * based on 4 patterns
 * (see PixBuffer_orderedDither)
 */
const double ditherMatrix[16] = {
    -0.875, 0.125, -0.625, 0.375,
    0.625, -0.375, 0.875, -0.125,
    -0.5, 0.5, -0.75, 0.25,
    1.0, 0.0, 0.75, -0.25
};

PixBuffer* PixBuffer_initPixBuffer(uint32_t width, uint32_t height)
{
    PixBuffer* newBuffer = (PixBuffer*)malloc(sizeof(PixBuffer));
    newBuffer->pixels = (uint32_t*)malloc(sizeof(uint32_t)*width*height);
    newBuffer->width = width;
    newBuffer->height = height;
    return newBuffer;
}

void PixBuffer_delPixBuffer(PixBuffer* buffer)
{
    free(buffer->pixels);
    free(buffer);
}

/** PixBuffer_drawColumn
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
    ////#pragma omp parallel for schedule(dynamic,1)
    for (int32_t i = y; i < y + h; i++)
    {
        PixBuffer_drawPix(buffer, x, i, PixBuffer_toPixColor(color.r,color.g,color.b,color.a));
    }
}

/** PixBuffer_drawRow
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
            PixBuffer_drawPix(buffer, i, y, PixBuffer_toPixColor(r,g,b,0xff));
        }
    }
}

/** PixBuffer_drawRect
 * @brief Draws a filled rectangle to a pixel buffer
 *
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

void PixBuffer_mergeBuffer(PixBuffer* target, PixBuffer* source, double alpha)
{
    uint32_t sourcePix;
    for (uint32_t i = 0; i < source->height; i++)
    {
        if (i < target->height)
        {
            for (uint32_t j = 0; j < source->width; j++)
            {
                if (j < target->width)
                {
                    sourcePix = source->pixels[j+i*source->width];
                    PixBuffer_drawPixAlpha(target, j, i, sourcePix, alpha);
                }
            }
        }
    }
}

void PixBuffer_fillBuffer(PixBuffer* target, uint32_t color, double alpha)
{
    for (uint32_t i = 0; i < target->height; i++)
    {
        if (i < target->height)
        {
            for (uint32_t j = 0; j < target->width; j++)
            {
                if (j < target->width)
                {
                    PixBuffer_drawPixAlpha(target, j, i, color, alpha);
                }
            }
        }
    }
}

void PixBuffer_drawBuffOffset(PixBuffer* target, PixBuffer* source, uint32_t x, uint32_t y, int32_t xOff)
{
    int32_t xCoord;
    for (uint32_t i = 0; i < source->height; i++)
    {
        if (i < target->height)
        {
            for (uint32_t j = 0; j < source->width; j++)
            {
                if (j < target->width)
                {
                    xCoord = (j + xOff) % target->width;
                    target->pixels[j+i*target->width] = source->pixels[xCoord+i*target->width];
                }
            }
        }
    }
}

/** PixBuffer_clearBuffer
 * @brief Clears buffer array to 0x00
 * * Useful if you need to quickly reuse a buffer
 * * for drawing layers/graphics updates. Sets to
 * * transparent black using memset
 * @param buffer PixBuffer struct to clear
 **/
void PixBuffer_clearBuffer(PixBuffer* buffer)
{
    memset(buffer->pixels, 0, buffer->width * buffer->height * 4);
}

/** PixBuffer_paletteFilter
 * @brief Remaps RGB buffer colors to a given pallette
 * * Note: it is important to ensure paletteNum is no longer than
 * * the palette list, otherwise your this will index nonexistant colors
 * * and make your output look really funky. And possibly segfault I guess
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

/** getNearestColor
 * @brief Internal function called by orderedDither for quantitization
 * 
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
 * PixBuffer_orderDither
 * The algorithm for this is somewhat based on the pseudocode
 * from the wikipedia page, but adapted once I found out
 * how this works
 * @brief Applies an ordered dither effect to a buffer
 *
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

/** to8BitColor
 * @brief Paletizes 32bit color to 8bit color
 * 
 * @param colorDat Raw truecolor value to paletize
 * @return 8 bit color value
 */
uint32_t to8BitColor(uint32_t colorDat)
{
    int r = (int)(colorDat >> 3*8);
    int g = (int)((colorDat >> 2*8) & 0xFF);
    int b = (int)((colorDat >> 8) & 0xFF);
    int newR = (int)ceil(round((double)r / 255.0*15) * (255.0/15));
    int newG = (int)ceil(round((double)g / 255.0*15) * (255.0/15));
    int newB = (int)ceil(round((double)b / 255.0*15) * (255.0/15));
    return (uint32_t)(newR) << 3*8 | (uint32_t)(newG) << 2*8 | (uint32_t)newB << 8 | (uint32_t)0xFF;
}

/** PixBuffer_orderDither256
 * Uses matrix dithering to palletize truecolor buffer to
 * 8-bit 256 color pallette
 * @brief Applies 256 color dithering filter to buffer
 * 
 * @param buffer PixBuffer to apply filter to
 * @param scaleFactor Stength of dithering. Multiplies values in 
 *        matrix to increase extremety of offsets
 **/
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

/** PixBuffer_monochromeFilter
 * * Note: Does not check fade percentage, could overflow color values
 * @brief Monochrome filter with selectable target color and saturation
 * @param buffer PixBuffer to apply filter to
 * @param targetColor Color to adjust chrominance towards
 * @param fadePercent Degree of monochromatic-ness (inverse saturation)
 **/
void PixBuffer_monochromeFilter(PixBuffer* buffer, SDL_Color targetColor, double fadePercent)
{
    SDL_Color oldColor;
    int targetAvg;
    uint32_t newColor;

    double targetR = targetColor.r/255.0;
    double targetG = targetColor.g/255.0;
    double targetB = targetColor.b/255.0;

    int dr;
    int dg;
    int db;

    for (int y = 0; y < buffer->height; y++)
    {
        for (int x = 0; x < buffer->width; x++)
        {
            oldColor = PixBuffer_toSDLColor(PixBuffer_getPix(buffer, x, y));
            targetAvg = (oldColor.r + oldColor.g + oldColor.b) / 3;
            dr = (targetAvg * targetR - oldColor.r) * fadePercent;
            dg = (targetAvg * targetG - oldColor.g) * fadePercent;
            db = (targetAvg * targetB - oldColor.b) * fadePercent;
            newColor = PixBuffer_toPixColor((uint8_t)(oldColor.r + dr), (uint8_t)(oldColor.g + dg), (uint8_t)(oldColor.b + db), (uint8_t)oldColor.a);
            PixBuffer_drawPix(buffer, x, y, newColor);
        }
    }
}

/** PixBuffer_inverseFilter
 * @brief Inverts the RGB channels of all pixels in a PixBuffer
 * @param buffer PixBuffer to swap channels of
 **/
void PixBuffer_inverseFilter(PixBuffer* buffer)
{
    SDL_Color oldColor;
    uint32_t newColor;

    int r;
    int g;
    int b;

    for (int y = 0; y < buffer->height; y++)
    {
        for (int x = 0; x < buffer->width; x++)
        {
            oldColor = PixBuffer_toSDLColor(PixBuffer_getPix(buffer, x, y));
            r = (255 - oldColor.r);
            g = (255 - oldColor.g);
            b = (255 - oldColor.b);
            newColor = PixBuffer_toPixColor((uint8_t)r, (uint8_t)g, (uint8_t)b, (uint8_t)oldColor.a);
            PixBuffer_drawPix(buffer, x, y, newColor);
        }
    }
}

/** PixBuffer_toPixColor
 * @brief Returns color formatted to RGBA format
 * @param r SDL_Color red component
 * @param g SDL_Color green component
 * @param b SDL_Color blue component
 * @param a SDL_Color alpha component
 **/
uint32_t PixBuffer_toPixColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return ((uint32_t)r << 3*8 | (uint32_t)g << 2*8 | (uint32_t)b << 8 | (uint32_t)a);
}

SDL_Color PixBuffer_toSDLColor(uint32_t pixColor)
{
    int r = (int)(pixColor >> 3*8);
    int g = (int)((pixColor >> 2*8) & 0xFF);
    int b = (int)((pixColor >> 8) & 0xFF);
    int a = (int)(pixColor & 0xFF);
    SDL_Color newColor = {r, g, b, a};
    return newColor;
}

uint32_t PixBuffer_blendAlpha(uint32_t baseColor, uint32_t addColor, double alphaNum)
{
    SDL_Color newSDLColor;
    uint32_t newColor;
    int addR = (int)(addColor >> 3*8);
    int addG = (int)((addColor >> 2*8) & 0xFF);
    int addB = (int)((addColor >> 8) & 0xFF);
    int addA = (int)(addColor & 0xFF);
    if (alphaNum*addA != 0 && alphaNum*addA != 255) // Alpha transparency, compute alpha based on array colors
    {
        double alpha = ((double)addA)/255.0 * (alphaNum);
        int oldR = (int)(baseColor >> 3*8);
        int oldG = (int)((baseColor >> 2*8) & 0xFF);
        int oldB = (int)((baseColor >> 8) & 0xFF);
        int oldA = (int)(baseColor & 0xFF);
        newSDLColor.r = (int)((double)addR * alpha + (double)oldR * (1-alpha));
        newSDLColor.g = (int)((double)addG * alpha + (double)oldG * (1-alpha));
        newSDLColor.b = (int)((double)addB * alpha + (double)oldB * (1-alpha));
        if (oldA == 255)
        {
            newSDLColor.a = 255;
        }
        else
        {
            newSDLColor.a = (int)((double)addA * alpha + (double)oldA * (1-alpha));
        }
        newColor = PixBuffer_toPixColor(newSDLColor.r, newSDLColor.g, newSDLColor.b, newSDLColor.a);
    }
    else
    {
        newColor = baseColor;
    }
    return newColor;
}

uint32_t PixBuffer_getPix(PixBuffer* buffer, uint32_t x, uint32_t y)
{
    return buffer->pixels[x + y * buffer->width];
}

uint32_t PixBuffer_getTex(RayTex* texture, uint8_t tileNum, uint32_t x, uint32_t y)
{
    return texture->pixData[(tileNum*texture->tileHeight + y) * texture->tileWidth + x];
}

/** PixBuffer_drawPix
 * @brief Draws a single pixel to the PixBuffer
 * @param buffer PixBuffer to draw to
 * @param x x coordinate of pixel
 * @param y y coordinate of pixel
 **/
void PixBuffer_drawPix(PixBuffer* buffer, uint32_t x, uint32_t y, uint32_t color)
{
    if (x < buffer->width && y < buffer->height)
    {
        buffer->pixels[y*buffer->width+x] = color;
    }
}

void PixBuffer_drawPixAlpha(PixBuffer* buffer, uint32_t x, uint32_t y, uint32_t color, double alphaNum)
{
    int r = (int)(color >> 3*8);
    int g = (int)((color >> 2*8) & 0xFF);
    int b = (int)((color >> 8) & 0xFF);
    int a = (int)(color & 0xFF);
    if (a)
    {
        if (alphaNum*a != 0 && alphaNum*a != 255) // Alpha transparency, compute alpha based on array colors
        {
            double alpha = ((double)a)/255.0 * (alphaNum);
            uint32_t oldPix = buffer->pixels[y*buffer->width+x];
            int oldR = (int)(oldPix >> 3*8);
            int oldG = (int)((oldPix >> 2*8) & 0xFF);
            int oldB = (int)((oldPix >> 8) & 0xFF);
            int oldA = (int)(oldPix & 0xFF);
            r = (int)((double)r * alpha + (double)oldR * (1-alpha));
            g = (int)((double)g * alpha + (double)oldG * (1-alpha));
            b = (int)((double)b * alpha + (double)oldB * (1-alpha));
            a = (int)((double)a * alpha + (double)oldA * (1-alpha));
        }
        PixBuffer_drawPix(buffer, x, y, PixBuffer_toPixColor(r,g,b,a));
    }
}

void PixBuffer_drawPixDouble(PixBuffer* buffer, double x, double y, uint32_t color, double alphaNum)
{
    uint32_t baseX = (uint32_t)floor(x);
    uint32_t baseY = (uint32_t)floor(y);
    double partX = x - baseX;
    double partY = y - baseY;
    if (x >= 0 && y >= 0)
    {
        PixBuffer_drawPixAlpha(buffer, baseX, baseY, color, alphaNum);
    }
    if (partX > 0.5)
    {
        baseX++;
        PixBuffer_drawPixAlpha(buffer, baseX, baseY, color, alphaNum);
    }
    else if (partX < -0.5)
    {
        baseX--;
        PixBuffer_drawPixAlpha(buffer, baseX, baseY, color, alphaNum);
    }
    if (partY > 0.5)
    {
        baseY++;
        PixBuffer_drawPixAlpha(buffer, baseX, baseY, color, alphaNum);
    }
    else if (partY < -0.5)
    {
        baseY--;
        PixBuffer_drawPixAlpha(buffer, baseX, baseY, color, alphaNum);
    }
    //PixBuffer_drawPixAlpha(buffer, baseX, baseY, color, alphaNum);
}

// RAYTEX FUNCTIONS
RayTex* RayTex_initFromRGBA(uint8_t* rgbaData, uint32_t tileWidth, uint32_t tileHeight, uint8_t numTiles)
{
    RayTex* newTex = (RayTex*)malloc(sizeof(RayTex));
    newTex->tileWidth = tileWidth;
    newTex->tileHeight = tileHeight;
    newTex->tileCount = numTiles;
    // Convert color chars into pixel ints
    newTex->pixData = 
        (uint32_t*)malloc(sizeof(uint32_t)*tileWidth*tileHeight*numTiles);
    uint32_t newPix = 0;
    for (uint32_t p = 0; p < tileWidth * tileHeight * numTiles; p++)
    {
        // Get each component
        for (uint8_t comp = 0; comp < 4; comp++)
        {
            newPix |= ((uint32_t)(rgbaData[p*4+comp]) << (8 * (3-comp)));
        }
        newTex->pixData[p] = newPix;
        newPix = 0;
    }
    return newTex;
}

void RayTex_delRayTex(RayTex* tex)
{
    free(tex->pixData);
    free(tex);
}
