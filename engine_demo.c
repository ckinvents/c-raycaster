#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gameengine.h"
#include "pixrender.h"
#include "assets/asset_list.h"

#define SCALE 4
const unsigned int WIDTH = WIDDERSHINS/SCALE;
const unsigned int HEIGHT = TURNWISE/SCALE;
#define MAP_SCALE 1
#define MAP_WIDTH 10
#define MAP_HEIGHT 13

// Test renderer
SDL_Renderer* renderer = NULL;
SDL_Texture* drawTex = NULL;
SDL_Window* window = NULL;
SDL_Event event;

SDL_Color BLACK = {0,0,0,255};



int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	const uint8_t* keys = SDL_GetKeyboardState(NULL);

	window = SDL_CreateWindow(
		"Raycaster Thing",
		30, 30,
		WIDTH*SCALE, HEIGHT*SCALE,
		SDL_WINDOW_OPENGL
	);

	uint32_t runTime = 0;
	double dt = 0;

	unsigned char testMapChar[MAP_WIDTH*MAP_HEIGHT] = {
		2,2,2,2,2,3,3,3,3,3,
		2,0,0,0,2,3,0,0,0,3,
		2,0,5,0,2,3,0,0,0,3,
		2,0,0,0,0,0,0,0,0,3,
		2,2,2,2,2,3,3,0,3,3,
		6,6,6,6,6,0,4,0,4,0,
		6,0,0,0,6,0,4,0,4,0,
		6,0,1,0,6,0,4,0,4,0,
		0,0,0,0,6,4,4,0,4,4,
		6,0,0,0,6,4,0,0,0,4,
		0,0,0,0,0,0,0,0,0,4,
		6,0,0,0,6,4,0,0,0,4,
		6,6,0,6,6,4,4,4,4,4
	};

	static const uint32_t box_data[256] = {
		0xa1c4b4ff, 0xa1c4b4ff, 0xa1c4b4ff, 0xa1c4b4ff, 0xa1c4b4ff, 0xa1c4b4ff, 0xa1c4b4ff, 0xa1c4b4ff, 0xa1c4b4ff, 0xa1c4b4ff, 0xa1c4b4ff, 0xa1c4b4ff, 0xa1c4b4ff, 0xa1c4b4ff, 0xa1c4b4ff, 0xa1c4b4ff, 
		0xa1c4b4ff, 0x5bab85ff, 0xa1c4b4ff, 0x5bab85ff, 0xa1c4b4ff, 0x5bab85ff, 0xa1c4b4ff, 0x5bab85ff, 0xa1c4b4ff, 0x5bab85ff, 0xa1c4b4ff, 0x5bab85ff, 0xa1c4b4ff, 0x5bab85ff, 0x3c6b9aff, 0x293c84ff, 
		0xa1c4b4ff, 0x3c6b9aff, 0x5bab85ff, 0x3c6b9aff, 0x5bab85ff, 0x293c84ff, 0x5bab85ff, 0x3c6b9aff, 0x5bab85ff, 0x3c6b9aff, 0x5bab85ff, 0x293c84ff, 0x5bab85ff, 0x293c84ff, 0x293c84ff, 0x261a10ff, 
		0x261a10ff, 0x293c84ff, 0x293c84ff, 0x5bab85ff, 0x293c84ff, 0x261a10ff, 0x293c84ff, 0x5bab85ff, 0x3c6b9aff, 0x5bab85ff, 0x293c84ff, 0x261a10ff, 0x293c84ff, 0x261a10ff, 0x261a10ff, 0x5a1e63ff, 
		0xcfbbaaff, 0x261a10ff, 0x261a10ff, 0x293c84ff, 0x261a10ff, 0x936138ff, 0x261a10ff, 0x293c84ff, 0x5bab85ff, 0x293c84ff, 0x261a10ff, 0x261a10ff, 0x261a10ff, 0x5a1e63ff, 0x936138ff, 0x5a1e63ff, 
		0xcfbbaaff, 0x936138ff, 0xcfbbaaff, 0x261a10ff, 0x936138ff, 0x936138ff, 0x936138ff, 0x261a10ff, 0x293c84ff, 0x261a10ff, 0x261a10ff, 0xcfbbaaff, 0x261a10ff, 0x261a10ff, 0x5a1e63ff, 0x5a1e63ff, 
		0xcfbbaaff, 0x936138ff, 0x936138ff, 0x936138ff, 0x936138ff, 0x936138ff, 0x936138ff, 0x936138ff, 0x261a10ff, 0x5a1e63ff, 0x261a10ff, 0xcfbbaaff, 0xcfbbaaff, 0xcfbbaaff, 0x261a10ff, 0x261a10ff, 
		0xcfbbaaff, 0x5a1e63ff, 0x5a1e63ff, 0x936138ff, 0x5a1e63ff, 0x936138ff, 0x936138ff, 0x936138ff, 0x5a1e63ff, 0x5a1e63ff, 0x261a10ff, 0xcfbbaaff, 0xcfbbaaff, 0x936138ff, 0xcfbbaaff, 0xcfbbaaff, 
		0x261a10ff, 0x261a10ff, 0x261a10ff, 0x5a1e63ff, 0x936138ff, 0x5a1e63ff, 0x936138ff, 0x5a1e63ff, 0x936138ff, 0x5a1e63ff, 0x261a10ff, 0xcfbbaaff, 0x936138ff, 0xcfbbaaff, 0x936138ff, 0x5a1e63ff, 
		0xcfbbaaff, 0xcfbbaaff, 0xcfbbaaff, 0x261a10ff, 0x5a1e63ff, 0x5a1e63ff, 0x5a1e63ff, 0x5a1e63ff, 0x5a1e63ff, 0x261a10ff, 0xcfbbaaff, 0xcfbbaaff, 0xcfbbaaff, 0x936138ff, 0x936138ff, 0x5a1e63ff, 
		0xcfbbaaff, 0xcfbbaaff, 0x936138ff, 0xcfbbaaff, 0x261a10ff, 0x261a10ff, 0x261a10ff, 0x5a1e63ff, 0x5a1e63ff, 0x261a10ff, 0xcfbbaaff, 0xcfbbaaff, 0x936138ff, 0xcfbbaaff, 0x936138ff, 0x5a1e63ff, 
		0xcfbbaaff, 0x936138ff, 0xcfbbaaff, 0x936138ff, 0xcfbbaaff, 0xcfbbaaff, 0xcfbbaaff, 0x261a10ff, 0x261a10ff, 0xcfbbaaff, 0xcfbbaaff, 0x936138ff, 0x936138ff, 0x936138ff, 0x5a1e63ff, 0x5a1e63ff, 
		0xcfbbaaff, 0xcfbbaaff, 0x936138ff, 0xcfbbaaff, 0x936138ff, 0x936138ff, 0x936138ff, 0xcfbbaaff, 0x261a10ff, 0xcfbbaaff, 0x936138ff, 0xcfbbaaff, 0x936138ff, 0x5a1e63ff, 0x936138ff, 0x5a1e63ff, 
		0xcfbbaaff, 0x936138ff, 0x936138ff, 0x936138ff, 0x936138ff, 0x936138ff, 0x5a1e63ff, 0x5a1e63ff, 0x261a10ff, 0xcfbbaaff, 0x936138ff, 0x936138ff, 0x936138ff, 0x936138ff, 0x5a1e63ff, 0x5a1e63ff, 
		0xcfbbaaff, 0x936138ff, 0x936138ff, 0x5a1e63ff, 0x936138ff, 0x5a1e63ff, 0x936138ff, 0x5a1e63ff, 0x261a10ff, 0xcfbbaaff, 0x936138ff, 0x5a1e63ff, 0x936138ff, 0x5a1e63ff, 0x5a1e63ff, 0x5a1e63ff, 
		0xcfbbaaff, 0x5a1e63ff, 0x5a1e63ff, 0x5a1e63ff, 0x5a1e63ff, 0x5a1e63ff, 0x5a1e63ff, 0x5a1e63ff, 0x261a10ff, 0xcfbbaaff, 0x5a1e63ff, 0x5a1e63ff, 0x5a1e63ff, 0x5a1e63ff, 0x5a1e63ff, 0x5a1e63ff
	};

	/*unsigned char testMapChar[MAP_WIDTH*MAP_HEIGHT] = {
		1,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0,
		0,0,0,0,0
	};*/

	SDL_Color colorKey[4] = {
		{255, 255, 255, 255},
		{220,50,50,255},	// Light red
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
	SDL_Color gameboyColorPalette[4] = {
		{0x33,0x2c,0x50,0xff},
		{0x46,0x87,0x8f,0xff},
		{0x94,0xe3,0x44,0xff},
		{0xe2,0xf3,0xe4,0xff}
	};
	SDL_Color solarizedDarkPalette[16] = {
		{0x00,0x2b,0x36,255},
		{0x07,0x36,0x42,255},
		{0x58,0x6e,0x75,255},
		{0x65,0x7b,0x83,255},
		{0x83,0x94,0x96,255},
		{0x93,0xa1,0xa1,255},
		{0xee,0xe8,0xd5,255},
		{0xfd,0xf6,0xe3,255},
		{0xb5,0x89,0x00,255},
		{0xcb,0x4b,0x16,255},
		{0xdc,0x32,0x2f,255},
		{0xd3,0x36,0x82,255},
		{0x6c,0x71,0xc4,255},
		{0x26,0x8b,0xd2,255},
		{0x2a,0xa1,0x98,255},
		{0x85,0x99,0x00,255},
	}; // Might as well
	SDL_Color testPalette[16] = {
		{0x00,0x00,0x00,0xFF},
		{0xFC,0xFC,0xFC,0xFF},
		{0xFC,0x78,0x00,0xFF},
		{0x78,0x00,0xFC,0xFF},
		{0x00,0xFC,0xA8,0xFF},
		{0x78,0x44,0x00,0xFF},
		{0x44,0x00,0x78,0xFF},
		{0x00,0x78,0x44,0xFF},
		{0x34,0x20,0x00,0xFF},
		{0x20,0x00,0x34,0xFF},
		{0x00,0x34,0x20,0xFF},
		{0x10,0x08,0x00,0xFF},
		{0x08,0x00,0x10,0xFF},
		{0x00,0x10,0x08,0xFF},
		{0xA8,0x34,0x00,0xFF},
		{0x34,0x00,0xA8,0xFF}
	};
	int palletteColorNum = 16;


	//Demo map
	Map testMap;
	RayEngine_generateMap(&testMap, testMapChar, MAP_WIDTH, MAP_HEIGHT, 2, colorKey, 4);
	uint32_t mapPixels[WIDTH*HEIGHT];
	PixBuffer mapBuffer;
	mapBuffer.pixels = mapPixels;
	mapBuffer.width = WIDTH;
	mapBuffer.height = HEIGHT;

	// Demo texture
	RayTex boxTex;
	boxTex.pixData = box_data;
	boxTex.tileCount = 1;
	boxTex.tileHeight = 16;
	boxTex.tileWidth = 16;
	RayTex mapTex;
	mapTex.pixData = blox_data;
	mapTex.tileCount = 8;
	mapTex.tileHeight = 16;
	mapTex.tileWidth = 16;

	// Shadow texture
	RayTex shadowTex;
	shadowTex.pixData = shadow_data;
	shadowTex.tileCount = 1;
	shadowTex.tileHeight = 32;
	shadowTex.tileWidth = 128;

	// Initialize sprite assets
	RayTex spriteTexs[9];
	// spriteTexs[0].pixData = cono_data;
	// spriteTexs[0].tileCount = 1;
	// spriteTexs[0].tileHeight = 128;
	// spriteTexs[0].tileWidth = 128;
	// spriteTexs[1].pixData = droptips_data;
	// spriteTexs[1].tileCount = 1;
	// spriteTexs[1].tileHeight = 128;
	// spriteTexs[1].tileWidth = 128;
	// spriteTexs[2].pixData = keule_data;
	// spriteTexs[2].tileCount = 1;
	// spriteTexs[2].tileHeight = 128;
	// spriteTexs[2].tileWidth = 105;
	// spriteTexs[3].pixData = lors_data;
	// spriteTexs[3].tileCount = 1;
	// spriteTexs[3].tileHeight = 128;
	// spriteTexs[3].tileWidth = 122;
	// spriteTexs[4].pixData = spaaace_data;
	// spriteTexs[4].tileCount = 1;
	// spriteTexs[4].tileHeight = 122;
	// spriteTexs[4].tileWidth = 128;
	// spriteTexs[5].pixData = thonking_data;
	// spriteTexs[5].tileCount = 1;
	// spriteTexs[5].tileHeight = 121;
	// spriteTexs[5].tileWidth = 128;
	// spriteTexs[6].pixData = udxs_data;
	// spriteTexs[6].tileCount = 1;
	// spriteTexs[6].tileHeight = 128;
	// spriteTexs[6].tileWidth = 120;
	spriteTexs[7].pixData = spinning_thonk_data;
	spriteTexs[7].tileCount = 30;
	spriteTexs[7].tileHeight = 128;
	spriteTexs[7].tileWidth = 128;
	spriteTexs[8].pixData = ball_data;
	spriteTexs[8].tileCount = 1;
	spriteTexs[8].tileWidth = 31;
	spriteTexs[8].tileHeight = 32;


	// Demo player
	double angleValues[WIDTH];
	Player testPlayer;
	GameEngine_initPlayer(&testPlayer, 1.5, 1.5, 0, M_PI/2, 5.0, WIDTH);

	// Demo spritelist with sprites
	Entity entityList[10];
	uint8_t numEntities = 10;
	GameEngine_initEntity(&entityList[0], 0, 0, 0, 0, &spriteTexs[7], &shadowTex);
	for (int s = 1; s < 9; s++)
	{
		GameEngine_initEntity(&entityList[s], 0, 0, 0, 0, &spriteTexs[7], &shadowTex);
		GameEngine_scaleEntity(&entityList[s], 0.5);
	}
	GameEngine_initEntity(&entityList[9], 7.5, 10.5, -0.375, 0, &spriteTexs[8], &shadowTex); // Test ball -0.375
	GameEngine_scaleEntity(&entityList[9], 0.25); //0.25
	entityList[9].sprite.alphaNum = 0.5;
	GameEngine_moveEntity(&entityList[0], 2.5, 7.5, 1); // Big Thonker

	// Allocate depth buffer 
	RayBuffer rayBuffer[WIDTH];
	for (int i = 0; i < WIDTH; i++)
	{
		rayBuffer[i].numLayers = 0;
	}

	// SDL renderer initialization
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	SDL_RenderSetScale(renderer, SCALE, SCALE);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	// Pixbuffer initialization
	drawTex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
	SDL_SetTextureBlendMode(drawTex, SDL_BLENDMODE_BLEND);
	uint32_t pixels[WIDTH * HEIGHT];
	PixBuffer buffer;
	buffer.pixels = pixels;
	buffer.width = WIDTH;
	buffer.height = HEIGHT;
	SDL_Rect screenRect = {0,0,WIDTH,HEIGHT};

	// Generate background texture
	uint32_t texPixels[WIDTH * HEIGHT];
	SDL_Rect gradientRectTop = {0,0,WIDTH,HEIGHT/2};
	//SDL_Color colorTop1 = {189,255,255,255};//{255,100,100,255};
	//SDL_Color colorTop2 = {77,150,154,255};//{0,0,100,255};
	SDL_Color colorTop1 = {0x5c,0x57,0xff,255};
	SDL_Color colorTop2 = {0xff,0x40,0x00,255};
	SDL_Rect gradientRectBottom = {0,HEIGHT/2,WIDTH,HEIGHT/2};
	//SDL_Color colorBottom1 = {159,197,182,255};
	//SDL_Color colorBottom2 = {79,172,135,255};
	SDL_Color colorBottom1 = {50,50,100,255};
	SDL_Color colorBottom2 = {150,150,190,255};
	PixBuffer_clearBuffer(&buffer);
	PixBuffer_drawHorizGradient(&buffer,&gradientRectTop, colorTop1, colorTop2);
	PixBuffer_drawHorizGradient(&buffer,&gradientRectBottom, colorBottom1, colorBottom2);
	// Note: between 15 & 20 is good for 16 color palettes
	PixBuffer_orderDither256(&buffer, 5);
	memcpy(texPixels, buffer.pixels, sizeof(uint32_t) * WIDTH * HEIGHT);

	uint8_t quit = 0;
	uint8_t frameCounter = 0;
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
		GameEngine_updatePlayer(&testPlayer, &testMap, dt);
		// Sprite movement
		for (int s = 1; s < 9; s++)
		{
			GameEngine_moveEntity(&entityList[s], 2.5 + cos((double)runTime/1000+(s-1)*M_PI/4), 7.5 + sin((double)runTime/1000+(s-1)*M_PI/4), (s-1) * 0.2);
			entityList[s].sprite.frameNum = (runTime)%30;
		}
		entityList[0].sprite.frameNum = 29-(runTime/100)%30;

		// Clear, draw line and update
		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(renderer);
		SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
		PixBuffer_clearBuffer(&buffer);
		memcpy(buffer.pixels, texPixels, sizeof(uint32_t) * WIDTH * HEIGHT);
		//RayEngine_raycastRender(&buffer, testPlayer, WIDTH, HEIGHT, &testMap, 0.01);
		RayEngine_raycastCompute(rayBuffer, &(testPlayer.camera), WIDTH, HEIGHT, &testMap, 0.01, &mapTex);
		// Update & draw sprites
		for (uint8_t s = 0; s < numEntities; s++)
		{
			GameEngine_updateEntity(&entityList[s]);
			RayEngine_raySpriteCompute(rayBuffer, &(testPlayer.camera), WIDTH, HEIGHT, 0.01, entityList[s].sprite);
			RayEngine_raySpriteCompute(rayBuffer, &(testPlayer.camera), WIDTH, HEIGHT, 0.01, entityList[s].shadow);
		}
		RayEngine_texRaycastRender(&buffer, WIDTH, HEIGHT, rayBuffer, testPlayer.camera.dist);
		PixBuffer_orderDither256(&buffer, 5);
		// Note: between 4 & 10 is good for 16 color palette
		SDL_UpdateTexture(drawTex, NULL, buffer.pixels, sizeof(uint32_t) * WIDTH);
		SDL_RenderCopy(renderer, drawTex, NULL, NULL);
		//PixBuffer_clearBuffer(&buffer);
		//RayEngine_drawMinimap(&buffer, testPlayer, WIDTH, HEIGHT, &testMap, 2);
		//PixBuffer_orderDither256(&buffer, 5);
		//SDL_UpdateTexture(drawTex, NULL, buffer.pixels, sizeof(uint32_t) * WIDTH);
		//SDL_RenderCopy(renderer, drawTex, NULL, NULL);
		SDL_RenderPresent(renderer);
		dt = 0.001 * (double)(SDL_GetTicks() - runTime);
		//printf("FPS: %d\n", 1.0/dt);
	}


	// Clean up and quit
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	renderer = NULL;
	window = NULL;
	SDL_Quit();
	return 0;
}