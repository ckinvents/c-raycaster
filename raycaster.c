#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

const unsigned int WIDTH = 320/4;
const unsigned int HEIGHT = 1000/4;

// Test renderer
SDL_Renderer* renderer = NULL;
SDL_Window* window = NULL;
SDL_Event event;

typedef struct _Player {
	double x;
	double y;
	double angle;
	double dist;
	double fov;
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

void raycastRender(SDL_Renderer* renderer,  Player* player, uint32_t width, uint32_t height, Map* map, double resolution, double angleArray[])
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
					SDL_SetRenderDrawColor(renderer, (int)((double)colorDat.r * (colorGrad)), (int)((double)colorDat.g * (colorGrad)), (int)((double)colorDat.b * (colorGrad)), 0xFF);
					double drawHeight = (double)(height / (depth * 10));
					SDL_RenderDrawLine(renderer, i, (int)((double)height / 2.0 - drawHeight), i, (int)((double)height / 2.0 + drawHeight));
					// Ray visualizer
					//SDL_SetRenderDrawColor(renderer, (int)(255.0 * ((double)i / width)), 0, 0, 0xFF);
					//SDL_RenderDrawLine(renderer, 0, 0, (int)(100 * (rayLen) * cos(rayAngle - startAngle)), (int)(100 * (rayLen) * sin(rayAngle - startAngle)));
				}
				else
				{
					SDL_SetRenderDrawColor(renderer, colorDat.r, colorDat.g, colorDat.b, colorDat.a);
					SDL_RenderDrawLine(renderer, i, 0, i, height);
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

void drawMinimap(SDL_Renderer* renderer, Player* player, unsigned int width, unsigned int height, Map* map, int blockSize)
{
	SDL_Rect mapRect;
	mapRect.w = map->width * blockSize;
	mapRect.h = map->height * blockSize;
	mapRect.x = width - mapRect.w;
	mapRect.y = 0;
	SDL_Rect blockRect;
	blockRect.w = blockSize;
	blockRect.h = blockSize;

	SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
	SDL_RenderFillRect(renderer, &mapRect);
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_RenderDrawRect(renderer, &mapRect);
	for (int i = 0; i < map->height; i++)
	{
		for (int j = 0; j < map->width; j++)
		{
			if (map->data[i * map->width + j] != 0)
			{
				blockRect.x = mapRect.x + j * blockSize;
				blockRect.y = mapRect.y + i * blockSize;
				SDL_Color blockColor = map->colorData[map->data[i * map->width + j] - 1];
				SDL_SetRenderDrawColor(renderer, blockColor.r, blockColor.g, blockColor.b, blockColor.a);
				SDL_RenderFillRect(renderer, &blockRect);
			}
		}
	}
	// Draws view fulcrum
	//SDL_RenderDrawLine(renderer, player->x * blockSize + mapRect.x, player->y * blockSize + mapRect.y, (player->x + player->dist * cos(player->angle - player->fov / 2)) * blockSize + mapRect.x, (player->y + player->dist * sin(player->angle - player->fov / 2)) * blockSize + mapRect.y);
	//SDL_RenderDrawLine(renderer, player->x * blockSize + mapRect.x, player->y * blockSize + mapRect.y, (player->x + player->dist * cos(player->angle + player->fov / 2)) * blockSize + mapRect.x, (player->y + player->dist * sin(player->angle + player->fov / 2)) * blockSize + mapRect.y);
	SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
	SDL_RenderDrawPoint(renderer, player->x * blockSize + mapRect.x, player->y * blockSize + mapRect.y);
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
		{255, 0, 0, 255},
		{0, 255, 0, 255},
		{0, 0, 255, 255}
	};

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
	}

	double angleValues[WIDTH];
	generateAngleValues(WIDTH, testPlayer->fov, angleValues);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	SDL_RenderSetScale(renderer, 4, 4);

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
		raycastRender(renderer, testPlayer, WIDTH, HEIGHT, &testMap, 0.005, angleValues);
		drawMinimap(renderer, testPlayer, WIDTH, HEIGHT, &testMap, 2);
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