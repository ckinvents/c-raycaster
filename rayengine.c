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

void RayEngine_drawMinimap(PixBuffer* buffer, Player* player, unsigned int width, unsigned int height, Map* map, int blockSize)
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
	//SDL_RenderDrawLine(renderer, player->x * blockSize + mapRect.x, player->y * blockSize + mapRect.y, (player->x + player->dist * cos(player->angle - player->fov / 2)) * blockSize + mapRect.x, (player->y + player->dist * sin(player->angle - player->fov / 2)) * blockSize + mapRect.y);
	//SDL_RenderDrawLine(renderer, player->x * blockSize + mapRect.x, player->y * blockSize + mapRect.y, (player->x + player->dist * cos(player->angle + player->fov / 2)) * blockSize + mapRect.x, (player->y + player->dist * sin(player->angle + player->fov / 2)) * blockSize + mapRect.y);
	SDL_Color playerCol = {0xFF, 0xFF, 0xFF, 0xFF};
	PixBuffer_drawPix(buffer, player->x * blockSize + mapRect.x, player->y * blockSize + mapRect.y, playerCol);
}

void RayEngine_deleteMap(unsigned char** map, int width, int height)
{
	for (int i = 0; i < height; i++)
	{
		free(map[i]);
	}
	free(map);
}

void RayEngine_generateAngleValues(uint32_t width, Player* player)
{
	double adjFactor = (double)width / (2 * tan(player->fov / 2));
	player->angleValues[0] = atan((width / 2) / adjFactor) - atan((width / 2 - 1) / adjFactor);
	for (uint32_t i = 1; i < width; i++)
	{
		if(i < width / 2)
		{
			player->angleValues[i] = player->angleValues[i-1] + atan((width / 2 - i) / adjFactor) - atan((width / 2 - i - 1) / adjFactor);
		}
		else
		{
			player->angleValues[i] = player->angleValues[i-1] + atan((i + 1 - width / 2) / adjFactor) - atan((i - width / 2) / adjFactor);
		}
	}
}

void RayEngine_raycastRender(PixBuffer* buffer,  Player* player, uint32_t width, uint32_t height, Map* map, double resolution)
{
	// Establish starting angle and sweep per column
	double startAngle = player->angle - player->fov / 2.0;
	double adjFactor = width / (2 * tan(player->fov / 2));
	double scaleFactor = (double)width / (double)height * 2.4;
	double rayAngle = startAngle;
	// Sweeeeep for each column
	#pragma omp parallel for schedule(dynamic,1) private(rayAngle)
	for (int i = 0; i < width; i++)
	{
		rayAngle = startAngle + player->angleValues[i];
		double rayX = player->x;
		double rayY = player->y;
		double rayStepX = (resolution) * cos(rayAngle);
		double rayStepY = (resolution) * sin(rayAngle);
		double stepLen = (resolution) / scaleFactor;
		long double rayLen = 0;
		int rayStep = 0;
		int rayOffX = 0;
		int rayOffY = 0;
		while (rayLen < player->dist)
		{
			int coordX = (int)floor(rayX+rayOffX);
			int coordY = (int)floor(rayY+rayOffY);
			if ((coordX >= 0.0 && coordY >= 0.0) && (coordX < map->width && coordY < map->height) && (map->data[coordY * map->width + coordX] != 0))
			{
				SDL_Color colorDat = map->colorData[map->data[coordY * map->width + coordX] - 1];
				if (rayLen != 0)
				{
					double newX;
					double newY;
					uint8_t side;
					double rayLen = sqrt(getInterDist(rayStepX, rayStepY, player->x + rayOffX, player->y + rayOffY, (double)coordX, (double)coordY, &newX, &newY, &side))/scaleFactor;
					double depth = (double)(rayLen * cos(rayAngle - player->angle));
					double colorGrad = (player->dist - depth) / player->dist;
					if (colorGrad < 0 || colorGrad > 1)
					{
						colorGrad = 0;
					}
					SDL_Color columnColor = {(int)((double)colorDat.r * (colorGrad)), (int)((double)colorDat.g * (colorGrad)), (int)((double)colorDat.b * (colorGrad)), 0xFF};
					double drawHeight = (double)(height / (depth * 10));
					PixBuffer_drawColumn(buffer, i, (int)((double)height / 2.0 - drawHeight), drawHeight*2, columnColor);
				}
				else
				{
					PixBuffer_drawColumn(buffer, i, 0, height, colorDat);
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

void RayEngine_texRaycastRender(PixBuffer* buffer, Player* player, uint32_t width, uint32_t height, Map* map, double resolution, RayTex* texData)
{
	// Establish starting angle and sweep per column
	double startAngle = player->angle - player->fov / 2.0;
	double adjFactor = width / (2 * tan(player->fov / 2));
	double scaleFactor = (double)width / (double)height * 2.4;
	double rayAngle = startAngle;
	// Sweeeeep for each column
	#pragma omp parallel for schedule(dynamic,1) private(rayAngle)
	for (int i = 0; i < width; i++)
	{
		rayAngle = startAngle + player->angleValues[i];
		double rayX = player->x;
		double rayY = player->y;
		double rayStepX = (resolution) * cos(rayAngle);
		double rayStepY = (resolution) * sin(rayAngle);
		double stepLen = (resolution) / scaleFactor;
		long double rayLen = 0;
		int rayStep = 0;
		int rayOffX = 0;
		int rayOffY = 0;
		while (rayLen < player->dist)
		{
			int coordX = (int)floor(rayX+rayOffX);
			int coordY = (int)floor(rayY+rayOffY);
			if ((coordX >= 0.0 && coordY >= 0.0) && (coordX < map->width && coordY < map->height) && (map->data[coordY * map->width + coordX] != 0))
			{
				SDL_Color colorDat = {0,0,0,255};
				if (rayLen != 0)
				{
					uint8_t side;
					double newX;
					double newY;
					double rayLen = sqrt(getInterDist(rayStepX, rayStepY, player->x + rayOffX, player->y + rayOffY, (double)coordX, (double)coordY, &newX, &newY, &side))/scaleFactor;
					uint32_t texCoord;
					if (side)
					{
						texCoord = (uint32_t)floor((newX - coordX) * texData->tileWidth);
					}
					else
					{
						texCoord = (uint32_t)floor((newY - coordY) * texData->tileWidth);
					}
					double depth = (double)(rayLen * cos(rayAngle - player->angle));
					double colorGrad = (depth) / player->dist;
					double drawHeight = (double)(height / (depth * 10));
					SDL_Color fadeColor = {77,150,154,255};
					double jumpHeight = 1;//2 + sin(SDL_GetTicks()/1000.0);
					PixBuffer_drawTexColumn(buffer, i, (int)(((double)height / 2.0 - drawHeight)/jumpHeight + height * (1.0 - 1.0/jumpHeight)), (int)drawHeight*2, texData, texCoord, colorGrad, fadeColor);
				}
				else
				{
					PixBuffer_drawColumn(buffer, i, 0, height, colorDat);
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
	
	if ((int)floor(leftCoord) == (int)coordY)
	{
		minDist = (xi - coordX)*(xi - coordX) + (yi - leftCoord)*(yi - leftCoord);
		*side = 0;
		*newX = coordX;
		*newY = leftCoord;
	}
	dist = (coordX + 1 - xi)*(coordX + 1 - xi) + (rightCoord - yi)*(rightCoord - yi);
	if ((int)floor(rightCoord) == (int)coordY && (dist < minDist || minDist == -1))
	{
		minDist = dist;
		*side = 0;
		*newX = coordX + 1;
		*newY = rightCoord;
	}
	dist = (xi - topCoord)*(xi - topCoord) + (yi - coordY)*(yi - coordY);
	if ((int)floor(topCoord) == (int)coordX && (dist < minDist || minDist == -1))
	{
		minDist = dist;
		*side = 1;
		*newX = topCoord;
		*newY = coordY;
	}
	dist = (xi - bottomCoord)*(xi - bottomCoord) + (yi - coordY - 1)*(yi - coordY - 1);
	if ((int)floor(bottomCoord) == (int)coordX && (dist < minDist || minDist == -1))
	{
		minDist = dist;
		*side = 1;
		*newX = bottomCoord;
		*newY = coordY + 1;
	}
	return minDist;
}

void RayEngine_updatePlayer(Player* player, Map* map, double dt)
{
	int borderWidth = 2;
	keys = SDL_GetKeyboardState(NULL);
	if (keys[SDL_SCANCODE_W]||keys[SDL_SCANCODE_S])
	{
		double dx = 2 * dt * cos(player->angle) / (2 - keys[SDL_SCANCODE_LSHIFT] * 1);
		double dy = 2 * dt * sin(player->angle) / (2 - keys[SDL_SCANCODE_LSHIFT] * 1);
		int oldX = (int)floor(player->x);
		int oldY = (int)floor(player->y);
		int newX;
		int newY;
		double changeX;
		double changeY;
		if (keys[SDL_SCANCODE_W])
		{
			newX = (int)floor(player->x+dx);
			changeX = dx;
			newY = (int)floor(player->y+dy);
			changeY = dy;
		}
		else
		{
			newX = (int)floor(player->x-dx);
			changeX = -dx;
			newY = (int)floor(player->y-dy);
			changeY = -dy;
		}
		if (newX < -map->border)
		{
			newX += map->width + map->border * 2;
			changeX += map->width + map->border * 2;
		}
		else if (newX >= map->width + map->border)
		{
			newX -= map->width + map->border * 2;
			changeX -= map->width + map->border * 2;
		}
		if (newY < -map->border)
		{
			newY += map->height + map->border*2;
			changeY += map->height + map->border*2;
		}
		else if (newY >= map->height + map->border)
		{
			newY -= map->height + map->border*2;
			changeY -= map->height + map->border*2;
		}
		if (((newX < 0 || newX >= map->width) || (oldY < 0 || oldY >= map->height)) || map->data[oldY*map->width+newX] == 0)
				player->x += changeX;
		if (((newY < 0 || newY >= map->height) || (oldX < 0 || oldX >= map->width)) || map->data[newY*map->width+oldX] == 0)
				player->y += changeY;
	}
	if (keys[SDL_SCANCODE_A])
	{
		player->angle -= dt;
	}
	else if (keys[SDL_SCANCODE_D])
	{
		player->angle += dt;
	}
}