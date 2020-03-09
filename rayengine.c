/**
 * A basic retro-style raycasting game engine implemented
 * entirely in SDL2. Uses textures and renderer for
 * hardware acceleration. Also implements my PixBuffer
 * effects for most of the graphics, shaders, etc.
 * 
 * @author Connor Ennis
 * @date 11/1/2019
 **/

#include <math.h>
#include <omp.h>
#include "rayengine.h"
#include <stdio.h>

const uint8_t* keys;
double getInterDist(double dx, double dy, double xi, double yi, double coordX, double coordY, double* newX, double* newY, uint8_t* side);

void RayEngine_generateMap(Map* newMap, unsigned char* charList, int width, int height, int border, SDL_Color* colorData, int numColor)
{
	newMap->data = charList;
	newMap->width = width;
	newMap->height = height;
	newMap->colorData = colorData;
	newMap->numColor = numColor;
	newMap->border = border;
}

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

void RayEngine_draw2DSprite(PixBuffer* buffer, RaySprite sprite)
{
	// First, compute screen-space sprite dimensions
	int32_t screenWidth = (int32_t)((double)sprite.texture->tileWidth*sprite.scaleFactor);
	int32_t screenHeight = (int32_t)((double)sprite.texture->tileHeight*sprite.scaleFactor);
	int32_t startX = (int32_t)(sprite.x - screenWidth / 2);
	int32_t startY = (int32_t)(sprite.y - screenHeight / 2);
	// Then render sprite based on dimensions
	int32_t columnNum;
	for (int32_t i = startX; i < startX + screenWidth; i++)
	{
		if (i >= 0 && i < buffer->width)
		{
			columnNum = (int32_t)((double)sprite.texture->tileWidth * ((double)(i - startX)/screenWidth));
			SDL_Color dummyColor = {0,0,0,0};
			PixBuffer_drawTexColumn(buffer, i, startY, screenHeight, sprite.texture, sprite.frameNum, sprite.alphaNum, columnNum, 0, dummyColor);
		}
	}
}

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
	// Draws view fulcrum
	//SDL_RenderDrawLine(renderer, camera->x * blockSize + mapRect.x, camera->y * blockSize + mapRect.y, (camera->x + camera->dist * cos(camera->angle - camera->fov / 2)) * blockSize + mapRect.x, (camera->y + camera->dist * sin(camera->angle - camera->fov / 2)) * blockSize + mapRect.y);
	//SDL_RenderDrawLine(renderer, camera->x * blockSize + mapRect.x, camera->y * blockSize + mapRect.y, (camera->x + camera->dist * cos(camera->angle + camera->fov / 2)) * blockSize + mapRect.x, (camera->y + camera->dist * sin(camera->angle + camera->fov / 2)) * blockSize + mapRect.y);
	SDL_Color cameraCol = {0xFF, 0xFF, 0xFF, 0xFF};
	//PixBuffer_drawPix(buffer, camera->x * blockSize + mapRect.x, camera->y * blockSize + mapRect.y, cameraCol);
}

void RayEngine_deleteMap(unsigned char** map, int width, int height)
{
	for (int i = 0; i < height; i++)
	{
		free(map[i]);
	}
	free(map);
}

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

// Compare function for qsort
int raycastCompare(const void *buffer1, const void *buffer2)
{
	if ((((RayColumn*)buffer2)->depth - ((RayColumn*)buffer1)->depth) < 0.0)
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

void RayEngine_raySpriteCompute(RayBuffer* rayBuffer, Camera* camera, uint32_t width, uint32_t height, double resolution, RaySprite sprite)
{
	// Establish starting angle and sweep per column
	double startAngle = camera->angle - camera->fov / 2.0;
	//double adjFactor = width / (2 * tan(camera->fov / 2));
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
				if (i >= 0 && i < width && rayBuffer[i].numLayers < 255)
				{
					texCoord = (uint32_t)floor(((double)spriteColumn / (double)screenWidth) * sprite.texture->tileWidth);
					rayBuffer[i].layers[rayBuffer[i].numLayers].texture = sprite.texture;
					rayBuffer[i].layers[rayBuffer[i].numLayers].texCoord = texCoord;
					rayBuffer[i].layers[rayBuffer[i].numLayers].tileNum = sprite.frameNum;
					rayBuffer[i].layers[rayBuffer[i].numLayers].alphaNum = sprite.alphaNum;
					rayBuffer[i].layers[rayBuffer[i].numLayers].depth = spriteDist;
					rayBuffer[i].layers[rayBuffer[i].numLayers].yCoord = startY;
					rayBuffer[i].layers[rayBuffer[i].numLayers].height = screenHeight;
					rayBuffer[i].numLayers++;
				}
				spriteColumn++;
			}
		}
	}
}

void RayEngine_raycastCompute(RayBuffer* rayBuffer, Camera* camera, uint32_t width, uint32_t height, Map* map, double resolution, RayTex* texData)
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
					int32_t drawHeight = (int32_t)ceil((double)height / (depth * 5));
					int32_t wallHeight = (int32_t)ceil(-camera->h * height / (depth * 5));
					int32_t startY = (int32_t)ceil((double)height / 2 - (double)drawHeight / 2 - wallHeight);
					SDL_Color fadeColor = {77,150,154,255};
					//PixBuffer_drawTexColumn(buffer, i, (int)(((double)height / 2.0 - drawHeight)/jumpHeight + height * (1.0 - 1.0/jumpHeight)), (int)drawHeight*2, texData, texCoord, colorGrad, fadeColor);
					rayBuffer[i].layers[rayBuffer[i].numLayers].texture = texData;
					rayBuffer[i].layers[rayBuffer[i].numLayers].texCoord = texCoord;
					rayBuffer[i].layers[rayBuffer[i].numLayers].tileNum = mapTile - 1;
					rayBuffer[i].layers[rayBuffer[i].numLayers].alphaNum = 1;
					rayBuffer[i].layers[rayBuffer[i].numLayers].depth = rayLen;
					rayBuffer[i].layers[rayBuffer[i].numLayers].yCoord = startY;
					rayBuffer[i].layers[rayBuffer[i].numLayers].height = drawHeight;
					rayBuffer[i].numLayers++;
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
				else //Camera is in column
				{
					//PixBuffer_drawColumn(buffer, i, 0, height, colorDat);
					rayBuffer[i].layers[rayBuffer[i].numLayers].texture = NULL;
					rayBuffer[i].layers[rayBuffer[i].numLayers].texCoord = 0;
					rayBuffer[i].layers[rayBuffer[i].numLayers].tileNum = 0;
					rayBuffer[i].layers[rayBuffer[i].numLayers].alphaNum = 0;
					rayBuffer[i].layers[rayBuffer[i].numLayers].depth = 0;
					rayBuffer[i].layers[rayBuffer[i].numLayers].yCoord = 0;
					rayBuffer[i].layers[rayBuffer[i].numLayers].height = 0;
					rayBuffer[i].numLayers++;
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

void RayEngine_texRaycastRender(PixBuffer* buffer, uint32_t width, uint32_t height, RayBuffer* rayBuffer, double renderDepth)
{
	SDL_Color fadeColor = {50,50,100,255};
	SDL_Color colorDat = {0,0,0,0};
	// For each screenwidth column
	for (int i = 0; i < width; i++)
	{
		// Sort ray depth values
		// (Furthest to closest, draw-order)
		qsort(rayBuffer[i].layers, rayBuffer[i].numLayers, sizeof(RayColumn), raycastCompare);
		// for every column in buffer...
		if (!rayBuffer[i].layers[0].texture)
		{
			PixBuffer_drawColumn(buffer, i, 0, height, colorDat);
		}
		else
		{
			for (int j = 0; j < rayBuffer[i].numLayers; j++)
			{
				double colorGrad;
				double fogConstant = 1.5/5;
				if (rayBuffer[i].layers[j].depth < (renderDepth*fogConstant))
				{
					colorGrad = (rayBuffer[i].layers[j].depth) / (renderDepth*fogConstant);
				}
				else
				{
					colorGrad = 1.0;
				}
				PixBuffer_drawTexColumn(buffer, i, rayBuffer[i].layers[j].yCoord, rayBuffer[i].layers[j].height, rayBuffer[i].layers[j].texture, rayBuffer[i].layers[j].tileNum, rayBuffer[i].layers[j].alphaNum, rayBuffer[i].layers[j].texCoord, colorGrad, fadeColor);
			}
		}
		rayBuffer[i].numLayers = 0;
	}
}

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

void RayEngine_texRenderFloor(PixBuffer* buffer, Camera* camera, uint32_t width, uint32_t height, Map* groundMap, double resolution, RayTex* texData)
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

	SDL_Color fadeColor = {50,50,100,255};
	
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
				uint32_t pixColor = texData->pixData[3 * texData->tileWidth * texData->tileHeight + texX + texY * texData->tileWidth];
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

void RayEngine_texRenderCeiling(PixBuffer* buffer, Camera* camera, uint32_t width, uint32_t height, Map* ceilingMap, RayTex* texData)
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

	SDL_Color fadeColor = {50,50,100,255};
	
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
				uint32_t pixColor = texData->pixData[4 * texData->tileWidth * texData->tileHeight + texX + texY * texData->tileWidth];
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
