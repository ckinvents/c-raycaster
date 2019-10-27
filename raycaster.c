#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "render.h"

const unsigned int WIDTH = 800/4;
const unsigned int HEIGHT = 640/4;

// Test renderer
SDL_Renderer* renderer = NULL;
SDL_Texture* drawTex = NULL;
SDL_Window* window = NULL;
SDL_Event event;

SDL_Color BLACK = {0,0,0,255};

typedef struct _Player {
	double x;
	double y;
	double angle;
	double dist;
	double fov;
	SDL_Color* colorPallette;
	int palletteSize;
} Player;

typedef struct _Map {
	unsigned char* data;
	SDL_Color* colorData;
	int numColor;
	int width;
	int height;
} Map;

void generateMap(Map* newMap, unsigned char* charList, int width, int height, SDL_Color* colorData, int numColor)
{
	newMap->data = charList;
	newMap->width = width;
	newMap->height = height;
	newMap->colorData = colorData;
	newMap->numColor = numColor;
}

void deleteMap(unsigned char** map, int width, int height)
{
	for (int i = 0; i < height; i++)
	{
		free(map[i]);
	}
	free(map);
}

double adjustedColorDiff(int r1, int g1, int b1, int r2, int g2, int b2)
{
	double rFact = 0.26;
	double gFact = 0.55;
	double bFact = 0.19;
	double bright1 = sqrt((rFact * r1) * (rFact * r1) + (gFact * g1) * (gFact * g1) + (bFact * b1) * (bFact * b1));
	double bright2 = sqrt((rFact * r2) * (rFact * r2) + (gFact * g2) * (gFact * g2) + (bFact * b2) * (bFact * b2));

	double baseDif = sqrt((rFact * (r1 - r2)) * (rFact * (r1 - r2)) + (rFact * (g1 - g2)) * (rFact * (g1 - g2)) + (rFact * (b1 - b2)) * (rFact * (b1 - b2)));
	double brightDif = abs(bright1 - bright2);

	return 0.5 * (brightDif - baseDif) + baseDif;
}

void setPalletteRenderColor(PixBuffer* buffer, int r, int g, int b, int a, SDL_Color* colorPallette, int numColor)
{
	int colNum = 0;
	uint32_t minColorDif = 0xFF*0xFF*3;//adjustedColorDiff(r, g, b, colorPallette[0].r, colorPallette[0].g, colorPallette[0].b);
	for (int i = 1; i < numColor; i++)
	{
		uint32_t colorDif = (uint32_t)(colorPallette[i].r - r)*(colorPallette[i].r - r) + (colorPallette[i].g - g)*(colorPallette[i].g - g) + (colorPallette[i].b - b)*(colorPallette[i].b - b);
		double rFact = 0.26;
		double bFact = 0.55;
		double gFact = 0.19;
		//double colorDif = adjustedColorDiff(r, g, b, colorPallette[i].r, colorPallette[i].g, colorPallette[i].b);//(uint32_t)(rFact * (double)(colorPallette[i].r - r))*(rFact * (double)(colorPallette[i].r - r)) + (gFact * (double)(colorPallette[i].g - g))*(gFact * (double)(colorPallette[i].g - g)) + (bFact * (double)(colorPallette[i].b - b))*(bFact * (double)(colorPallette[i].b - b));
		if (colorDif < minColorDif)
		{
			minColorDif = colorDif;
			colNum = i;
		}
	}
	PixBuffer_setColor(buffer, colorPallette[colNum].r, colorPallette[colNum].g, colorPallette[colNum].b, colorPallette[colNum].a);
}

void applyShaders(SDL_Renderer* renderer, uint32_t width, uint32_t height, SDL_Color* palette, int paletteSize)
{
	// Get pixel data from renderer
	void* data = malloc(sizeof(uint8_t) * 4);
	SDL_Rect coords = {0,0,1,1};
	for (uint32_t x = 0; x < width; x++)
	{
		for (uint32_t y = 0; y < height; y++)
		{
			//free(data);
			coords.x = x;
			coords.y = y;
			SDL_RenderReadPixels(renderer, &coords, SDL_PIXELFORMAT_RGBA8888, data, 4);
			printf("Coords: %u, %u\nColordata: %x\n", coords.x, coords.y, *(uint32_t*)data);
			uint8_t pR = (uint8_t)(*((uint32_t*)data) & 0xFF000000 >> 3*8);
			uint8_t pG = (uint8_t)(*((uint32_t*)data) & 0x00FF0000 >> 2*8);
			uint8_t pB = (uint8_t)(*((uint32_t*)data) & 0x0000FF00 >> 1*8);
			printf("Extracted coords: %x%x%x\n", pR, pG, pB);
			//SDL_SetRenderDrawColor(renderer, pR/2, pG/2, pB/2, 255);
			//SDL_RenderDrawPoint(renderer, x, y);
		}
	}
	free(data);
}

void raycastRender(PixBuffer* buffer,  Player* player, uint32_t width, uint32_t height, Map* map, double resolution, double angleArray[])
{
	// Establish starting angle and sweep per column
	double startAngle = player->angle - player->fov / 2.0;
	double adjFactor = width / (2 * tan(player->fov / 2));
	double scaleFactor = (double)WIDTH / (double)HEIGHT * 2.4;
	double rayAngle = startAngle;
	// Sweeeeep for each column
	for (int i = 0; i < width; i++)
	{
		rayAngle += angleArray[i];
		double rayX = player->x;
		double rayY = player->y;
		long double rayLen = 0;
		int rayStep = 0;
		while (rayLen < player->dist)
		{
			int coordX = (int)floor(rayX);
			int coordY = (int)floor(rayY);
			if ((rayX >= 0.0 && rayY >= 0.0) && (coordX < map->width && coordY < map->height) && (map->data[coordY * map->width + coordX] != 0))
			{
				SDL_Color colorDat = map->colorData[map->data[coordY * map->width + coordX] - 1];
				if (rayLen != 0)
				{
					double depth = (double)(rayLen * cos(rayAngle - player->angle));
					double colorGrad = (player->dist - depth) / player->dist;
					if (colorGrad < 0 || colorGrad > 1)
					{
						colorGrad = 0;
					}
					//printf("colorGrad: %f\n", colorGrad);
					//if (colorGrad > 1.0)
					//{
					//	colorGrad = 0;
					//}
					//printf("Depth: %f\n", depth);
					if (!player->colorPallette)
					{
						PixBuffer_setColor(buffer, (int)((double)colorDat.r * (colorGrad)), (int)((double)colorDat.g * (colorGrad)), (int)((double)colorDat.b * (colorGrad)), 0xFF);
					}
					else
					{
						setPalletteRenderColor(buffer, (int)((double)colorDat.r * (colorGrad)), (int)((double)colorDat.g * (colorGrad)), (int)((double)colorDat.b * (colorGrad)), 0xFF, player->colorPallette, player->palletteSize);
					}
					double drawHeight = (double)(height / (depth * 10));
					PixBuffer_drawColumn(buffer, i, (int)((double)height / 2.0 - drawHeight), drawHeight*2);
					// Ray visualizer
					//SDL_SetRenderDrawColor(renderer, (int)(255.0 * ((double)i / width)), 0, 0, 0xFF);
					//SDL_RenderDrawLine(renderer, 0, 0, (int)(100 * (rayLen) * cos(rayAngle - startAngle)), (int)(100 * (rayLen) * sin(rayAngle - startAngle)));
				}
				else
				{
					if (!player->colorPallette)
					{
						PixBuffer_setColor(buffer, colorDat.r, colorDat.g, colorDat.b, colorDat.a);
					}
					else
					{
						setPalletteRenderColor(buffer, colorDat.r, colorDat.g, colorDat.b, colorDat.a, player->colorPallette, player->palletteSize);
					}
					PixBuffer_drawColumn(buffer, i, 0, height);
				}
				break;
			}
			rayX += (resolution) * cos(rayAngle);
			rayY += (resolution) * sin(rayAngle);
			rayStep++;
			rayLen = sqrtl((rayX - player->x) * (rayX - player->x) + (rayY - player->y) * (rayY - player->y))/scaleFactor;
		}
	}
}

void generateAngleValues(uint32_t width, double fov, double angleArray[])
{
	double adjFactor = (double)width / (2 * tan(fov / 2));
	for (uint32_t i = 0; i < width; i++)
	{
		if(i < width / 2)
		{
			angleArray[i] = atan((width / 2 - i) / adjFactor) - atan((width / 2 - i - 1) / adjFactor);
		}
		else
		{
			angleArray[i] = atan((i + 1 - width / 2) / adjFactor) - atan((i - width / 2) / adjFactor);
		}
	}
}

void drawMinimap(PixBuffer* buffer, Player* player, unsigned int width, unsigned int height, Map* map, int blockSize)
{
	SDL_Rect mapRect;
	mapRect.w = map->width * blockSize;
	mapRect.h = map->height * blockSize;
	mapRect.x = width - mapRect.w;
	mapRect.y = 0;
	SDL_Rect blockRect;
	blockRect.w = blockSize;
	blockRect.h = blockSize;

	PixBuffer_setColor(buffer, 0x00, 0x00, 0x00, 0xFF);
	PixBuffer_drawRect(buffer, &mapRect);
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
				PixBuffer_setColor(buffer, blockColor.r, blockColor.g, blockColor.b, blockColor.a);
				PixBuffer_drawRect(buffer, &blockRect);
			}
		}
	}
	// Draws view fulcrum
	//SDL_RenderDrawLine(renderer, player->x * blockSize + mapRect.x, player->y * blockSize + mapRect.y, (player->x + player->dist * cos(player->angle - player->fov / 2)) * blockSize + mapRect.x, (player->y + player->dist * sin(player->angle - player->fov / 2)) * blockSize + mapRect.y);
	//SDL_RenderDrawLine(renderer, player->x * blockSize + mapRect.x, player->y * blockSize + mapRect.y, (player->x + player->dist * cos(player->angle + player->fov / 2)) * blockSize + mapRect.x, (player->y + player->dist * sin(player->angle + player->fov / 2)) * blockSize + mapRect.y);
	PixBuffer_setColor(buffer, 0xFF, 0xFF, 0xFF, 0xFF);
	PixBuffer_drawPix(buffer, player->x * blockSize + mapRect.x, player->y * blockSize + mapRect.y);
}

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	const uint8_t* keys = SDL_GetKeyboardState(NULL);

	window = SDL_CreateWindow(
		"Raycaster Thing",
		30, 30,
		WIDTH*4, HEIGHT*4,
		SDL_WINDOW_OPENGL
	);

	uint32_t runTime = 0;
	double dt = 0;

	unsigned char testMapChar[117] = {
		1,2,3,4,1,2,2,2,1,
		4,0,0,0,2,0,0,0,3,
		3,0,1,0,3,0,0,0,3,
		2,0,0,0,0,0,0,0,3,
		1,4,3,2,1,4,0,4,1,
		0,0,0,0,0,4,0,4,0,
		0,0,0,0,0,4,0,4,0,
		0,0,0,0,0,4,0,4,0,
		0,0,0,0,4,4,0,4,4,
		0,0,0,0,4,0,0,0,4,
		0,0,0,0,4,0,0,0,4,
		0,0,0,0,4,0,0,0,4,
		0,0,0,0,4,4,4,4,4,

	};

	SDL_Color colorKey[4] = {
		{255, 255, 255, 255},
		{255,70,70,255},	// Light red
		{100,255,70,255},	// Light green
		{70,70,255,255} 	// Light blue
	};

	// C64 color pallette, why not
	SDL_Color commodoreColorPallette[16] = {
		{0,0,0,255},		// Black
		{255,255,255,255},	// White
		{136,0,0,255},		// Red
		{170,255,238,255},	// Cyan
		{204,68,204,255},	// Purple
		{0,204,85,255},		// Green
		{0,0,170,255},		// Blue
		{238,238,119,255},	// Yellow
		{221,136,85,255},	// Orange
		{102,68,0,255},		// Brown
		{255,119,119,255},	// Light red
		{51,51,51,255},		// Dark grey
		{119,119,119,255},	// Grey
		{170,255,102,255},	// Light green
		{0,136,255,255},	// Light blue
		{187,187,187,255}	// Light grey
	};
	SDL_Color picoColorPallette[16] = {
		{0,0,0,255},
		{29,43,83,255},
		{126,37,83,255},
		{0,135,81,255},
		{171,82,54,255},
		{95,87,79,255},
		{194,195,199,255},
		{255,241,232,255},
		{255,0,77,255},
		{255,163,0,255},
		{255,236,39,255},
		{0,228,54,255},
		{41,173,255,255},
		{131,118,156,255},
		{255,119,168,255},
		{255,204,170,255},
	};
	int palletteColorNum = 16;

	Map testMap;
	generateMap(&testMap, testMapChar, 9, 13, colorKey, 4);

	Player* testPlayer = malloc(sizeof(Player));
	if (testPlayer)
	{
		testPlayer->angle = M_PI / 2.0;
		testPlayer->dist = 5.0;
		testPlayer->x = 2.5;
		testPlayer->y = 3.5;
		testPlayer->fov = M_PI/2;
		testPlayer->colorPallette = picoColorPallette;
		testPlayer->palletteSize = palletteColorNum;
	}

	double angleValues[WIDTH];
	generateAngleValues(WIDTH, testPlayer->fov, angleValues);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	SDL_RenderSetScale(renderer, 4, 4);

	drawTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, WIDTH, HEIGHT);
	uint32_t pixels[WIDTH * HEIGHT];
	PixBuffer buffer;
	buffer.pixels = pixels;
	buffer.width = WIDTH;
	buffer.height = HEIGHT;
	

	uint8_t quit = 0;
	while(!quit)
	{
		runTime = SDL_GetTicks();		
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				quit = 1;
			}
		}
		keys = SDL_GetKeyboardState(NULL);
		if (keys[SDL_SCANCODE_W])
		{
			testPlayer->x += dt * cos(testPlayer->angle) / (4 - keys[SDL_SCANCODE_LSHIFT] * 3);
			testPlayer->y += dt * sin(testPlayer->angle) / (4 - keys[SDL_SCANCODE_LSHIFT] * 3);
		}
		else if (keys[SDL_SCANCODE_S])
		{
			testPlayer->x -= dt * cos(testPlayer->angle) / (4 - keys[SDL_SCANCODE_LSHIFT] * 3);
			testPlayer->y -= dt * sin(testPlayer->angle) / (4 - keys[SDL_SCANCODE_LSHIFT] * 3);
		}
		if (keys[SDL_SCANCODE_A])
		{
			testPlayer->angle -= dt;
		}
		else if (keys[SDL_SCANCODE_D])
		{
			testPlayer->angle += dt;
		}

		// Clear, draw line and update
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
		PixBuffer_setColor(&buffer, 0, 0, 0, 0xFF);
		PixBuffer_clearBuffer(&buffer);
		raycastRender(&buffer, testPlayer, WIDTH, HEIGHT, &testMap, 0.005, angleValues);
		drawMinimap(&buffer, testPlayer, WIDTH, HEIGHT, &testMap, 2);
		SDL_UpdateTexture(drawTex, NULL, buffer.pixels, sizeof(uint32_t) * WIDTH);
		SDL_RenderCopy(renderer, drawTex, NULL, NULL);
		SDL_RenderPresent(renderer);

		dt = 0.001 * (double)(SDL_GetTicks() - runTime);
	}


	// Clean up and quit

	//deleteMap(testMap, testMapWidth, testMapHeight);
	free(testPlayer);
	//testMap = NULL;
	testPlayer = NULL;

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	renderer = NULL;
	window = NULL;
	SDL_Quit();
	return 0;
}