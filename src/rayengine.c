/**
 * A basic retro-style raycasting game engine implemented
 * entirely in SDL2. Uses textures and renderer for
 * hardware acceleration. Also implements my PixBuffer
 * effects for most of the graphics, shaders, etc.
 * 
 * @author Connor Ennis
 * @date 11/1/2019
 **/

#include "rayengine.h"

const static SDL_Color FOG_COLOR = {50,20,50,255};//{50,50,80,255}//{77,150,154,255}

const uint8_t* keys;
double getInterDist(double dx, double dy, double xi, double yi, double coordX, double coordY, double* newX, double* newY, uint8_t* side);

/** RayEngine_initDepthBuffer
 * @brief Initializes a new RayEngine depth buffer
 * 
 * @param buffer DepthBuffer pointer for new buffer
 * @param width x dimension of buffer in pixels
 * @param height y dimension of the buffer in pixels
 */
DepthBuffer* RayEngine_initDepthBuffer(uint32_t width, uint32_t height)
{
	DepthBuffer* newBuffer = (DepthBuffer*)malloc(sizeof(DepthBuffer));
	newBuffer->pixelBuffer = PixBuffer_initPixBuffer(width, height);
	newBuffer->alphaBuffer = PixBuffer_initPixBuffer(width, height);
	newBuffer->pixelDepth = (double*)malloc(sizeof(double) * width * height);
	newBuffer->alphaDepth = (double*)malloc(sizeof(double) * width * height);
	return newBuffer;
}

/** RayEngine_getDepth
 * @brief Retrieves the depth of a pixel in a DepthBuffer
 * 
 * @param buffer Buffer to retrieve depth from
 * @param x x coordinate of pixel
 * @param y y coordinate of pixel
 * @param layer BufferLayer to select alpha or opaque buffer layer
 * @return double Depth of pixel
 */
double RayEngine_getDepth(DepthBuffer* buffer, uint32_t x, uint32_t y, uint8_t layer)
{
	uint32_t width = buffer->pixelBuffer->width;
	return layer ? buffer->alphaDepth[width * y + x] : buffer->pixelDepth[width * y + x];
}

/** RayEngine_setDepth
 * @brief Sets the depth of a pixel in a DepthBuffer
 * 
 * @param buffer Buffer to set depth in
 * @param x x coordinate of pixel
 * @param y y coordinate of pixel
 * @param layer BufferLayer to select alpha or opaque buffer layer
 * @param depth Depth of pixel
 */
void RayEngine_setDepth(DepthBuffer* buffer, uint32_t x, uint32_t y, uint8_t layer, double depth)
{
	uint32_t width = buffer->pixelBuffer->width;
	if (layer)
	{
		buffer->alphaDepth[width * y + x] = depth;
	}
	else
	{
		buffer->pixelDepth[width * y + x] = depth;
	}
	
}

/** RayEngine_drawPix
 * @brief 
 * 
 * @param buffer 
 * @param x 
 * @param y 
 * @param color 
 * @param alphaNum 
 * @param depth 
 */
void RayEngine_drawPix(DepthBuffer* buffer, uint32_t x, uint32_t y, uint32_t color, double alphaNum, double depth)
{
	if (x >= 0 && x < buffer->pixelBuffer->width && y >= 0 && y < buffer->pixelBuffer->height)
	{
		// Keep pixel in alpha layer
		if (alphaNum < 1 || (color & 0xff) < 255)
		{
			// If new alpha in front
			double pixDepth = RayEngine_getDepth(buffer, x, y, BL_ALPHA);
			if (pixDepth > depth)
			{
				RayEngine_setDepth(buffer, x, y, BL_ALPHA, depth);
				if (pixDepth == INFINITY)
				{
					SDL_Color alphaColor = PixBuffer_toSDLColor(color);
					alphaColor.a *= alphaNum;
					PixBuffer_drawPix(buffer->alphaBuffer, x, y, PixBuffer_toPixColor(alphaColor.r, alphaColor.g, alphaColor.b, alphaColor.a));
				}
				else
				{
					PixBuffer_drawPixAlpha(buffer->alphaBuffer, x, y, color, alphaNum);
				}
			}
			else
			{
				PixBuffer_drawPix(buffer->alphaBuffer, x, y, PixBuffer_blendAlpha(color, PixBuffer_getPix(buffer->alphaBuffer, x, y), 1));
			}
		}
		// For opaque layer
		else
		{
			// If new pixel in front
			if (RayEngine_getDepth(buffer, x, y, BL_BASE) > depth)
			{
				RayEngine_setDepth(buffer, x, y, BL_BASE, depth);
				PixBuffer_drawPix(buffer->pixelBuffer, x, y, color);
			}
		}
	}
}

uint32_t RayEngine_pixGradientShader(uint32_t pixel, double percent, SDL_Color target)
{
	int r = (int)(pixel >> 3*8);
	int g = (int)((pixel >> 2*8) & 0xFF);
	int b = (int)((pixel >> 8) & 0xFF);
	int a = (int)(pixel & 0xFF);
	int dr = target.r - r;
	int dg = target.g - g;
	int db = target.b - b;
	int da = target.a - a;
	r += (int)((double)dr * percent);
	g += (int)((double)dg * percent);
	b += (int)((double)db * percent);
	a += (int)((double)da * percent);
	return PixBuffer_toPixColor(r,g,b,a);
}

void RayEngine_drawTexColumn(DepthBuffer* buffer, uint32_t x, int32_t y,
 							 int32_t h, double depth, RayTex* texture,
							 uint8_t tileNum, double alphaNum, 
							 uint32_t column, double fadePercent, 
							 SDL_Color targetColor)
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
    if (y + h > buffer->pixelBuffer->height)
    {
        h = buffer->pixelBuffer->height - y;
    }

    for (int32_t i = 0; i < h; i++)
    {
        // Calculate pixel to draw from texture
        uint32_t pix = texture->pixData[\
			tileNum*texture->tileWidth*texture->tileHeight + \
			(uint32_t)floor(((double)(offY + i)/(double)offH) * \
			(texture->tileHeight)) * texture->tileWidth + column];
		if (pix & 0xFF)
		{
			pix = RayEngine_pixGradientShader(pix, fadePercent, targetColor);
			RayEngine_drawPix(buffer, x, i+y, pix, alphaNum, depth);
    	}
	}
}

/** RayEngine_renderBuffer
 * @brief Merges opaque and alpha layers of buffer for rendering
 * 
 * @param buffer Buffer to render
 */
void RayEngine_renderBuffer(DepthBuffer* buffer)
{
	uint32_t width = buffer->pixelBuffer->width;
	uint32_t height = buffer->pixelBuffer->height;
	uint32_t pix;
	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{	
			if (RayEngine_getDepth(buffer, i, j, BL_BASE) > RayEngine_getDepth(buffer, i, j, BL_ALPHA))
			{
				pix = PixBuffer_getPix(buffer->alphaBuffer, i, j);
				PixBuffer_drawPixAlpha(buffer->pixelBuffer, i, j, pix, 1.0);
			}
		}
	}
}

void RayEngine_resetDepthBuffer(DepthBuffer* buffer)
{
	uint32_t width = buffer->pixelBuffer->width;
	uint32_t height = buffer->pixelBuffer->height;
	// Clear pixels
	// Reset depths
	for (uint64_t i = 0; i < (uint64_t)(width*height); i++)
	{
		buffer->pixelBuffer->pixels[i] = 0;
		buffer->alphaBuffer->pixels[i] = 0;
		buffer->pixelDepth[i] = INFINITY;
		buffer->alphaDepth[i] = INFINITY;
	}
}

/** RayEngine_delDepthBuffer
 * @brief Deallocates DepthBuffer memory
 * ! Will destroy buffer
 * @param buffer Buffer to free
 */
void RayEngine_delDepthBuffer(DepthBuffer* buffer)
{
	PixBuffer_delPixBuffer(buffer->pixelBuffer);
	PixBuffer_delPixBuffer(buffer->alphaBuffer);
	free(buffer->pixelDepth);
	free(buffer->alphaDepth);
	free(buffer);
}

//! Old
/** RayEngine_generateMap
 * @brief Generates new Map for game engine
 * TODO: Update generator, move to GameEngine
 * @param newMap Pointer to new Map
 * @param charList Map data formatted as string
 * TODO: Update map formatting, make real parser
 * @param width Width of map in grid units
 * @param height Height of map in grid units
 * @param border size of border in grid units (for tiles maps)
 * @param colorData Array of colors to render map tiles
 * ! Warning: depricated feature, pre-textures
 * @param numColor Number of colors in colorData
 * ! Warning: depricated feature, pre-textures
 */
void RayEngine_generateMap(Map* newMap, unsigned char* charList, int width, int height, int border, SDL_Color* colorData, int numColor)
{
	newMap->data = charList;
	newMap->width = width;
	newMap->height = height;
	newMap->colorData = colorData;
	newMap->numColor = numColor;
	newMap->border = border;
}

/** RayEngine_initSprite
 * @brief Initializes new RaySprite for engine
 * TODO: Better integrate with 2D sprites, add more metadata
 * TODO: Consolidate w/ GameEngine
 * @param newSprite Pointer to new RaySprite
 * @param texture RayTex representing sprite frame data
 * @param scaleFactor Default sprite size
 * @param alphaNum Default sprite transparency
 * @param x Map x coordinate
 * @param y Map y coordinate
 * @param h Map h coordinate
 */
void RayEngine_initSprite(RaySprite* newSprite, RayTex* texture, double scaleFactor, double alphaNum, double x, double y, double h)
{
	newSprite->texture = texture;
	newSprite->scaleFactor = scaleFactor;
	newSprite->alphaNum = alphaNum;
	newSprite->x = x;
	newSprite->y = y;
	newSprite->h = h;
	newSprite->frameNum = 0;
}

/** RayEngine_draw2DSprite
 * @brief Renders a 2D sprite to the screen
 * TODO: Consolidate w/ PixBuffer
 * TODO: Better manage w/ 3D sprites
 * @param buffer PixBuffer to render to
 * @param sprite RaySprite to render
 * @param angle Angle (rad) to render sprite at
 * TODO: Add to sprite metadata
 */
void RayEngine_draw2DSprite(PixBuffer* buffer, RaySprite sprite, double angle)
{
	// First, compute screen-space sprite dimensions
	double scaleWidth = sprite.texture->tileWidth*sprite.scaleFactor;
	double scaleHeight = sprite.texture->tileHeight*sprite.scaleFactor;
	int32_t boundSize = (uint32_t)(scaleWidth > scaleHeight ? scaleWidth*1.5 : scaleHeight*1.5);
	// Then render sprite based on dimensions
	int32_t texX;
	int32_t texY;
	double pixX;
	double pixY;
	uint32_t pixColor;
	// For all pixels in bounding box (yes, I know the box can be more dynamically sized but
	// I'm lazy and this will have to cut it for now)
	for (int32_t i = -boundSize/2; i < boundSize/2; i++)
	{
		for (int32_t j = -boundSize/2; j < boundSize/2; j++)
		{
			texX = (int32_t)round((i * cos(angle) - j * sin(angle)) / sprite.scaleFactor + sprite.texture->tileWidth/2.0 - 0.5);
			texY = (int32_t)round((i * sin(angle) + j * cos(angle)) / sprite.scaleFactor + sprite.texture->tileHeight/2.0 - 0.5);
			pixX = sprite.x + i;
			pixY = sprite.y + j;
			if (pixX >= 0 && pixX < buffer->width && pixY >= 0 && pixY < buffer->height &&\
				texX >= 0 && texX < sprite.texture->tileWidth && texY >=0 && texY < sprite.texture->tileHeight)
			{
				pixColor = PixBuffer_getTex(sprite.texture, 0, texX, texY);
				PixBuffer_drawPixAlpha(buffer, pixX, pixY, pixColor, sprite.alphaNum);
			}
		}
	}
}

//! Old
/** RayEngine_drawMinimap
 * @brief Renders minimap to screen
 * !! Very depricated code, does not work, cannot delete (???)
 * TODO: Please fix all of this
 * @param buffer PixBuffer to render minimap to
 * @param camera Active camera to render from
 * ! Also depricated, does not do anything
 * @param width Width of window
 * ! Redundant, pre-buffer size
 * @param height Height of window
 * ! Ditto
 * @param map Map to render
 * @param blockSize Size of tiles on rendered map
 */
void RayEngine_drawMinimap(PixBuffer* buffer, Camera* camera, unsigned int width, unsigned int height, Map* map, int blockSize)
{
	SDL_Rect mapRect;
	mapRect.w = map->width * blockSize;
	mapRect.h = map->height * blockSize;
	mapRect.x = width - mapRect.w;
	mapRect.y = 0;
	SDL_Rect blockRect;
	blockRect.w = blockSize;
	blockRect.h = blockSize;
	SDL_Color backColor = {0x00, 0x00, 0x00, 0x40};
	PixBuffer_drawRect(buffer, &mapRect, backColor);
	//SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	//SDL_RenderDrawRect(renderer, &mapRect);
	for (int i = 0; i < map->height; i++)
	{
		for (int j = 0; j < map->width; j++)
		{
			if (map->data[i * map->width + j] != 0)
			{
				blockRect.x = mapRect.x + j * blockSize;
				blockRect.y = mapRect.y + i * blockSize;
				SDL_Color blockColor = map->colorData[map->data[i * map->width + j] - 1];
				PixBuffer_drawRect(buffer, &blockRect, blockColor);
			}
		}
	}
}

//! Old
/** RayEngine_deleteMap
 * @brief Destroy Map
 * TODO: Update & move to GameEngine
 * !!! Very very very depricated, pre-color, does not do anything
 * @param map Pointer to map array (pre-color, no Map struct)
 * ! Do not feed it a Map struct
 * @param width Width of map (in tiles)
 * @param height Height of map (in tiles)
 */
void RayEngine_deleteMap(unsigned char** map, int width, int height)
{
	for (int i = 0; i < height; i++)
	{
		free(map[i]);
	}
	free(map);
}

/** RayEngine_generateAngleValues
 * @brief Creates a pregenerated list of angle offsets for Camera
 * TODO: Consolidate w/ Camera struct and GameEngine code 
 * @param width Width in pixels to generate offsets for
 * @param camera Camera for offsets
 */
void RayEngine_generateAngleValues(uint32_t width, Camera* camera)
{
	double adjFactor = (double)width / (2 * tan(camera->fov / 2));
	camera->angleValues[0] = atan((width / 2) / adjFactor) - atan((width / 2 - 1) / adjFactor);
	for (uint32_t i = 1; i < width; i++)
	{
		if(i < width / 2)
		{
			camera->angleValues[i] = camera->angleValues[i-1] + atan((width / 2 - i) / adjFactor) - atan((width / 2 - i - 1) / adjFactor);
		}
		else
		{
			camera->angleValues[i] = camera->angleValues[i-1] + atan((i + 1 - width / 2) / adjFactor) - atan((i - width / 2) / adjFactor);
		}
	}
}

//! RayBuffer dependent
/** RayEngine_draw3DSprite
 * @brief Renders 3D RaySprites for raycaster
 * TODO: Update w/ DepthBuffer, consolidate w/ sprite struct better
 * @param rayBuffer RayBuffer to render to
 * ! Depricated, see DepthBuffer
 * @param camera Camera to render from
 * TODO: Update to make less jank
 * @param width Window width (in pixels)
 * ! Redundant
 * @param height Window height (in pixels)
 * ! Redundant
 * @param resolution Raycast stepsize
 * TODO: Use camera or engine parameter instead 
 * @param sprite RaySprite to draw
 */
void RayEngine_draw3DSprite(DepthBuffer* buffer, Camera* camera, uint32_t width, uint32_t height, double resolution, RaySprite sprite)
{
	double scaleFactor = (double)width / (double)height * 2.4;
	// Generate screenspace angle mapping constant
	const double angleMapConstant = (double)(width) / (2*tan(camera->fov/2));
	// Render sprite to buffer
	double spriteAngle = atan2(sprite.y - camera->y, sprite.x - camera->x);
	double screenAngle = spriteAngle - camera->angle;
	//printf("Sprite %d screen angle: %f\n", s, screenAngle);
	double spriteDist = cos(screenAngle) * (sqrt((camera->x - sprite.x)*(camera->x - sprite.x) + (camera->y - sprite.y)*(camera->y - sprite.y))/scaleFactor);
	// Depth check, can't be on or behind camera
	if (spriteDist > 0)
	{
		// Compute column from screen angle
		int32_t centerX = (int32_t)floor(width / 2 + (int32_t)(angleMapConstant * tan(screenAngle)));
		// Get width and height
		int32_t screenHeight;
		int32_t screenWidth;
		if (sprite.texture->tileHeight >= sprite.texture->tileWidth)
		{
			screenHeight = (int32_t)((double)height / (spriteDist * 5) * sprite.scaleFactor);
			screenWidth = (int32_t)((double)screenHeight * ((double)sprite.texture->tileWidth / (double)sprite.texture->tileHeight));
		}
		else
		{
			screenWidth = (int32_t)ceil((double)height / (spriteDist * 5) * sprite.scaleFactor);
			screenHeight = (int32_t)ceil((double)screenWidth * ((double)sprite.texture->tileHeight / (double)sprite.texture->tileWidth));
		}
		
		int32_t spriteHeight = (int32_t)((sprite.h - camera->h) * height / (spriteDist * 5)); // I dunno why it's 40
		int32_t startX = centerX - screenWidth / 2;
		int32_t endX = startX + screenWidth;
		int32_t startY = (int32_t)ceil((height / 2) - ((double)screenHeight / 2) - spriteHeight);
		// Write to buffer if in fulcrum
		if (startX <= (int32_t)width && endX >= 0)
		{
			// Iterate through screen columns
			uint32_t spriteColumn = 0;
			uint32_t texCoord;
			for (int32_t i = startX; i < endX; i++)
			{
				if (i >= 0 && i < width)
				{
					double colorGrad;
					double fogConstant = 1.5/5;
					if (spriteDist < (camera->dist*fogConstant))
					{
						colorGrad = (spriteDist) / (camera->dist*fogConstant);
					}
					else
					{
						colorGrad = 1.0;
					}
					texCoord = (uint32_t)floor(((double)spriteColumn / (double)screenWidth) * sprite.texture->tileWidth);
					RayEngine_drawTexColumn(
						buffer, i, startY, screenHeight, spriteDist,
						sprite.texture, sprite.frameNum, sprite.alphaNum,
						texCoord, colorGrad, FOG_COLOR
					);
				}
				spriteColumn++;
			}
		}
	}
}

//! RayBuffer dependent
void RayEngine_raycastRender(DepthBuffer* buffer, Camera* camera, uint32_t width, uint32_t height, Map* map, double resolution, RayTex* texData)
{
	// Establish starting angle and sweep per column
	double startAngle = camera->angle - camera->fov / 2.0;
	double adjFactor = width / (2 * tan(camera->fov / 2));
	double scaleFactor = (double)width / (double)height * 2.4;
	double rayAngle = startAngle;
	// Sweeeeep for each column
	#pragma omp parallel for schedule(dynamic,1) private(rayAngle)
	for (int i = 0; i < width; i++)
	{
		rayAngle = startAngle + camera->angleValues[i];
		double rayX = camera->x;
		double rayY = camera->y;
		double rayStepX = (resolution) * cos(rayAngle);
		double rayStepY = (resolution) * sin(rayAngle);
		double stepLen = (resolution) / scaleFactor;
		double rayLen = 0;
		int rayStep = 0;
		int rayOffX = 0;
		int rayOffY = 0;
		int collisions = 0;
		while (rayLen < camera->dist && collisions < 3)
		{
			int coordX = (int)floor(rayX+rayOffX);
			int coordY = (int)floor(rayY+rayOffY);
			if ((coordX >= 0.0 && coordY >= 0.0) && (coordX < map->width && coordY < map->height) && (map->data[coordY * map->width + coordX] != 0))
			{
				uint8_t mapTile = map->data[coordY * map->width + coordX];
				uint32_t texCoord;
				SDL_Color colorDat = {0,0,0,255};
				if (rayLen != 0)
				{
					uint8_t side;
					double newX;
					double newY;
					double rayLen = sqrt(getInterDist(rayStepX, rayStepY, camera->x + rayOffX, camera->y + rayOffY, (double)coordX, (double)coordY, &newX, &newY, &side))/scaleFactor;
					uint32_t texCoord;
					if (side > 1)
					{
						texCoord = (uint32_t)floor((newX - coordX) * texData->tileWidth);
					}
					else
					{
						texCoord = (uint32_t)floor((newY - coordY) * texData->tileWidth);
					}
					double depth = (double)(rayLen * cos(rayAngle - camera->angle));
					//double colorGrad = (depth) / camera->dist;
					//* Note: This is an awful mess but it is a temporary fix to get around rounding issues
					int32_t drawHeight = (int32_t)ceil((double)height / (depth * 5));
					int32_t wallHeight = (int32_t)round(-camera->h * drawHeight);
					int32_t startY = height / 2 - drawHeight / 2 - wallHeight;
					int32_t offsetStartY = height / 2 - drawHeight / 2;
					int32_t deltaY = height - offsetStartY * 2;
					//SDL_Color fadeColor = {77,150,154,255};
					double colorGrad;
					double fogConstant = 1.5/5;
					if (rayLen < (camera->dist*fogConstant))
					{
						colorGrad = (rayLen) / (camera->dist*fogConstant);
					}
					else
					{
						colorGrad = 1.0;
					}
					RayEngine_drawTexColumn(
						buffer, i, startY, deltaY, depth,
						texData, mapTile - 1, 1.0, 
						texCoord, colorGrad, FOG_COLOR
					);
					// Check for texture column transparency
					uint8_t hasAlpha = 0;
					for (int p = 0; p < texData->tileHeight; p++)
					{
						if ((texData->pixData[(mapTile-1)*texData->tileWidth*texData->tileHeight+texCoord+(texData->tileWidth*p)] & 0xFF) < 0xFF)
						{
							collisions++;
							if (side == 0) // Hit from left
							{
								rayX += 1;
								rayY += rayStepY * (1.0/rayStepX);
							}
							else if (side == 1) // Hit from right
							{
								rayX -= 1;
								rayY -= rayStepY * (1.0/rayStepX);
							}
							else if (side == 2) // Hit from top
							{
								rayX += rayStepX * (1.0/rayStepY);
								rayY += 1;
							}
							else // Hit from bottom
							{
								rayX -= rayStepX * (1.0/rayStepY);
								rayY -= 1;
							}
							if (rayX+rayOffX < -map->border)
							{
								rayOffX += map->width + map->border * 2;
							}
							else if (rayX+rayOffX >= map->width + map->border)
							{
								rayOffX -= map->width + map->border * 2;
							}
							if (rayY+rayOffY < -map->border)
							{
								rayOffY += map->height + map->border*2;
							}
							else if (rayY+rayOffY >= map->height + map->border)
							{
								rayOffY -= map->height + map->border*2;
							}
							rayLen = sqrt((rayX-camera->x)*(rayX-camera->x) + (rayY-camera->y)*(rayY-camera->y));
							rayStep++;
							hasAlpha = 1;
							break;
						}
					}
					if (hasAlpha)
					{
						continue;
					}
				}
				break;
			}
			rayX += rayStepX;
			rayY += rayStepY;
			if (rayX+rayOffX < -map->border)
			{
				rayOffX += map->width + map->border * 2;
			}
			else if (rayX+rayOffX >= map->width + map->border)
			{
				rayOffX -= map->width + map->border * 2;
			}
			if (rayY+rayOffY < -map->border)
			{
				rayOffY += map->height + map->border*2;
			}
			else if (rayY+rayOffY >= map->height + map->border)
			{
				rayOffY -= map->height + map->border*2;
			}
			rayStep++;
			rayLen += stepLen;
		}
	}
}

/** getInterDist
 * @brief Compute linear interpolation of ray intersect with wall
 * 
 * @param dx x displacement along ray step
 * @param dy y displacement along ray step
 * @param xi Initial x coordinate
 * @param yi Initial y coordinate
 * @param coordX Grid tile x coordinate
 * @param coordY Grid tile y coordinate
 * @param newX Interpolated x coordinate
 * @param newY Interpolated y coordinate
 * @param side Numerical representation of side of intersect
 * @return double Adjusted distance to intersect
 */
double getInterDist(double dx, double dy, double xi, double yi, double coordX, double coordY, double* newX, double* newY, uint8_t* side)
{
	// Check side intercepts first
	double slope = (dy/dx);
	double leftCoord = slope * (coordX - xi) + yi;
	double rightCoord = slope * (coordX + 1 - xi) + yi;
	slope = (dx/dy);
	double topCoord = slope * (coordY - yi) + xi;
	double bottomCoord = slope * (coordY + 1 - yi) + xi;
	double dist;
	double minDist = -1;
	
	if ((int)floor(leftCoord) == (int)coordY) // Left side
	{
		minDist = (xi - coordX)*(xi - coordX) + (yi - leftCoord)*(yi - leftCoord);
		*side = 0;
		*newX = coordX;
		*newY = leftCoord;
	}
	dist = (coordX + 1 - xi)*(coordX + 1 - xi) + (rightCoord - yi)*(rightCoord - yi);
	if ((int)floor(rightCoord) == (int)coordY && (dist < minDist || minDist == -1)) // Right side
	{
		minDist = dist;
		*side = 1;
		*newX = coordX + 1;
		*newY = rightCoord;
	}
	dist = (xi - topCoord)*(xi - topCoord) + (yi - coordY)*(yi - coordY);
	if ((int)floor(topCoord) == (int)coordX && (dist < minDist || minDist == -1)) // Top side
	{
		minDist = dist;
		*side = 2;
		*newX = topCoord;
		*newY = coordY;
	}
	dist = (xi - bottomCoord)*(xi - bottomCoord) + (yi - coordY - 1)*(yi - coordY - 1);
	if ((int)floor(bottomCoord) == (int)coordX && (dist < minDist || minDist == -1)) // Bottom side
	{
		minDist = dist;
		*side = 3;
		*newX = bottomCoord;
		*newY = coordY + 1;
	}
	return minDist;
}

/** RayEngine_texRenderFloor
 * @brief Renders raycasted floor
 * TODO: Add depth
 * TODO: Make mappable
 * TODO: Make multilayer
 * TODO: Consolidate w/ floor renderer
 * @param buffer PixBuffer to render to
 * @param camera Camera to render from
 * @param width Width of pixbuffer in pixels
 * @param height Height of pixbuffer in pixels
 * @param groundMap Tile map to render from
 * ! Doesn't do anything atm
 * TODO: Integrate with main map
 * @param resolution ???
 * TODO: Remove, does nothing
 * @param texData Texture to render to floor
 * @param tileNum Number of tile from texture set to render
 * TODO: Placeholder, remove when mapping added
 */
void RayEngine_texRenderFloor(PixBuffer* buffer, Camera* camera, uint32_t width, uint32_t height, Map* groundMap, double resolution, RayTex* texData, uint8_t tileNum)
{
	double scaleFactor = (double)width / (double)height * 2.4;

	// Get initial coordinate position at top-left of floor space
	uint32_t startX = 0;
	uint32_t startY = height / 2;

	double pixelX;
	double pixelY;
	double pixelDist;
	double pixelDepth;
	double fadePercent;

	uint32_t texX;
	uint32_t texY;

	double startAngle = camera->angle - camera->fov / 2.0;
	double rayAngle;
	double rayCos;

	SDL_Color fadeColor = FOG_COLOR;
	
	// iterate through *all* pixels...
	for (int x = startX; x < width; x++)
	{
		// Establish angle of column...
		rayAngle = startAngle + camera->angleValues[x];
		rayCos = cos(rayAngle - camera->angle);

		for (int y = startY + 1; y < height; y++)
		{
			// Compute the distance to the pixel...
			pixelDist = (double)height * (1 + 2 * camera->h) / (10.0 * (y-startY-1) * rayCos) * scaleFactor;
			double fogConstant = 4.0/5;
			pixelDepth = (pixelDist * rayCos);
			fadePercent = pixelDist / (camera->dist * fogConstant);
			pixelX = camera->x + pixelDist * cos(rayAngle);
			pixelY = camera->y + pixelDist * sin(rayAngle);
			// Wow, is that really it? The math says so...
			int r;
			int g;
			int b;
			if (pixelDist < camera->dist * fogConstant)
			{
				// Get associated coordinate pixel...
				// TODO: some grid code...
				texX = (uint32_t)floor((double)texData->tileWidth * (pixelX - floor(pixelX)));
				texY = (uint32_t)floor((double)texData->tileHeight * (pixelY - floor(pixelY)));
				uint32_t pixColor = texData->pixData[tileNum * texData->tileWidth * texData->tileHeight + texX + texY * texData->tileWidth];
				r = (int)(pixColor >> 3*8);
				g = (int)((pixColor >> 2*8) & 0xFF);
				b = (int)((pixColor >> 8) & 0xFF);
				int dr = fadeColor.r - r;
				int dg = fadeColor.g - g;
				int db = fadeColor.b - b;
				r += (int)((double)dr * fadePercent);
				g += (int)((double)dg * fadePercent);
				b += (int)((double)db * fadePercent);
			}
			else
			{
				r = fadeColor.r;
				g = fadeColor.g;
				b = fadeColor.b;
			}
			buffer->pixels[x + y * buffer->width] = ((uint32_t)r << 3*8 | (uint32_t)g << 2*8 | (uint32_t)b << 8 | (uint32_t)0xFF);
		}
	}
}

/** RayEngine_texRenderCeiling
 * @brief Renders raycasted ceiling
 * TODO: See above (RayEngine_texRenderFloor)
 * @param buffer PixBuffer to render to
 * @param camera Camera to render from
 * @param width Width of pixbuffer in pixels
 * @param height Height of pixbuffer in pixels
 * @param groundMap Tile map to render from
 * ! Doesn't do anything atm
 * TODO: Integrate with main map
 * @param texData Texture to render to floor
 * @param tileNum Number of tile from texture set to render
 * TODO: Placeholder, remove when mapping added
 */
void RayEngine_texRenderCeiling(PixBuffer* buffer, Camera* camera, uint32_t width, uint32_t height, Map* ceilingMap, RayTex* texData, uint8_t tileNum)
{
	double scaleFactor = (double)width / (double)height * 2.4;

	// Get initial coordinate position at top-left of floor space
	uint32_t startX = 0;
	uint32_t startY = 0;

	double pixelX;
	double pixelY;
	double pixelDist;
	double pixelDepth;
	double fadePercent;

	uint32_t texX;
	uint32_t texY;

	double startAngle = camera->angle - camera->fov / 2.0;
	double rayAngle;
	double rayCos;

	SDL_Color fadeColor = FOG_COLOR;
	
	// iterate through *all* pixels...
	for (int x = startX; x < width; x++)
	{
		// Establish angle of column...
		rayAngle = startAngle + camera->angleValues[x];
		rayCos = cos(rayAngle - camera->angle);

		for (int y = startY; y < height / 2; y++)
		{
			// Compute the distance to the pixel...
			pixelDist = (double)height * (1 - 2 * camera->h) / (10.0 * (height / 2 - y) * rayCos) * scaleFactor;
			pixelDepth = (pixelDist * rayCos);
			double fogConstant = 4.0/5;
			fadePercent = pixelDist / (camera->dist * fogConstant);
			pixelX = camera->x + pixelDist * cos(rayAngle);
			pixelY = camera->y + pixelDist * sin(rayAngle);
			// Wow, is that really it? The math says so...
			int r;
			int g;
			int b;
			if (pixelDist < camera->dist * fogConstant)
			{
				// Get associated coordinate pixel...
				// TODO: some grid code...
				texX = (uint32_t)floor((double)texData->tileWidth * (pixelX - floor(pixelX)));
				texY = (uint32_t)floor((double)texData->tileHeight * (pixelY - floor(pixelY)));
				uint32_t pixColor = texData->pixData[tileNum * texData->tileWidth * texData->tileHeight + texX + texY * texData->tileWidth];
				r = (int)(pixColor >> 3*8);
				g = (int)((pixColor >> 2*8) & 0xFF);
				b = (int)((pixColor >> 8) & 0xFF);
				int dr = fadeColor.r - r;
				int dg = fadeColor.g - g;
				int db = fadeColor.b - b;
				r += (int)((double)dr * fadePercent);
				g += (int)((double)dg * fadePercent);
				b += (int)((double)db * fadePercent);
			}
			else
			{
				r = fadeColor.r;
				g = fadeColor.g;
				b = fadeColor.b;
			}
			PixBuffer_drawPix(buffer, x, y, PixBuffer_toPixColor(r,g,b,0xff));
		}
	}
}