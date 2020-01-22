/**
 * Game mechanics implementation for the 
 * rayengine. Handles things involving
 * sprites and players, such as collisions,
 * updates, events, and other game stuff.
 * Also animates memes I guess
 * 
 * @author Connor Ennis
 * @date 11/1/2019
 **/

#include <math.h>
#include "gameengine.h"

void GameEngine_initPlayer(Player* player, double x, double y, double angle, double fov, double viewDist, uint32_t screenWidth)
{
	player->x = x;
	player->y = y;
	player->angle = angle;
	player->health = 0; //PLACEHOLDER
	player->state = 0; //PLACEHOLDER
	player->spacePressed = 0;
	player->coolDown = 0;
	player->camera.dist = viewDist;
	player->camera.fov = fov;
	RayEngine_generateAngleValues(screenWidth, &(player->camera));
}

void GameEngine_initEntity(Entity* entity, double x, double y, double h, double angle, RayTex* spriteTex, RayTex* shadowTex)
{
	entity->x = x;
	entity->y = y;
	entity->h = h;
	entity->angle = angle;
	entity->health = 0; //PLACEHOLDER
	entity->state = 0; //PLACEHOLDER
	RayEngine_initSprite(&(entity->sprite), spriteTex, 1.0, 1.0, x, y, h);
	RayEngine_initSprite(&(entity->shadow), shadowTex, 0, 0.5, x, y, -0.5);
}

void GameEngine_initProjectiles(ProjectileList* projectiles, uint32_t numProjectile, RayTex* spriteTex, RayTex* shadowTex)
{
	for (int i = 0; i < numProjectile; i++)
	{
		GameEngine_initEntity(&projectiles->projectiles[i], -1000, -1000, -0.5, 0, spriteTex, shadowTex);
		projectiles->projectiles[i].sprite.scaleFactor = 0.1;
	}
}

void GameEngine_moveEntity(Entity* entity, double x, double y, double h)
{
	entity->x = x;
	entity->y = y;
	entity->h = h;
}

void GameEngine_scaleEntity(Entity* entity, double scaleFactor)
{
	entity->sprite.scaleFactor = scaleFactor;
}

void GameEngine_updatePlayer(Player* player, Map* map, double dt)
{
	int borderWidth = 2;
	uint8_t* keys = SDL_GetKeyboardState(NULL);
	if ((keys[SDL_SCANCODE_W]||keys[SDL_SCANCODE_S]||keys[SDL_SCANCODE_Q]||keys[SDL_SCANCODE_E])&&!((keys[SDL_SCANCODE_W]&&keys[SDL_SCANCODE_S])||keys[SDL_SCANCODE_Q]&&keys[SDL_SCANCODE_E]))
	{
		double dx;
		double dy;
		double speedFactor = (1 + keys[SDL_SCANCODE_LSHIFT] * 1.5);
		if ((keys[SDL_SCANCODE_W]||keys[SDL_SCANCODE_S])&&!(keys[SDL_SCANCODE_Q]||keys[SDL_SCANCODE_E]))
		{
			dx = 2 * dt * cos(player->angle) * speedFactor;
			dy = 2 * dt * sin(player->angle) * speedFactor;
		}
		else if ((keys[SDL_SCANCODE_W]&&keys[SDL_SCANCODE_Q])||(keys[SDL_SCANCODE_S]&&keys[SDL_SCANCODE_E]))
		{
			dx = 2 * dt * cos(player->angle-M_PI/4) * speedFactor;
			dy = 2 * dt * sin(player->angle-M_PI/4) * speedFactor;
		}
		else if ((keys[SDL_SCANCODE_S]&&keys[SDL_SCANCODE_Q])||(keys[SDL_SCANCODE_W]&&keys[SDL_SCANCODE_E]))
		{
			dx = 2 * dt * cos(player->angle+M_PI/4) * speedFactor;
			dy = 2 * dt * sin(player->angle+M_PI/4) * speedFactor;
		}
		else
		{
			dx = 2 * dt * cos(player->angle+M_PI/2) * speedFactor;
			dy = 2 * dt * sin(player->angle+M_PI/2) * speedFactor;
		}
		int oldX = (int)floor(player->x);
		int oldY = (int)floor(player->y);
		int newX;
		int newY;
		double changeX;
		double changeY;
		if ((keys[SDL_SCANCODE_W]||(keys[SDL_SCANCODE_E])&&!(keys[SDL_SCANCODE_S])))
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
	if (player->angle >= (2 * M_PI))
	{
		player->angle -= (2 * M_PI);
	}
	else if (player->angle < 0)
	{
		player->angle += (2 * M_PI);
	}
	player->camera.x = player->x;
	player->camera.y = player->y;
	player->camera.angle = player->angle;
}

void GameEngine_updateEntity(Entity* entity)
{
	entity->sprite.x = entity->x;
	entity->sprite.y = entity->y;
	entity->sprite.h = entity->h;
	entity->shadow.x = entity->x;
	entity->shadow.y = entity->y;
	entity->shadow.scaleFactor = exp(-(entity->h+0.5))*(entity->sprite.scaleFactor);
}

void GameEngine_updateProjectile(ProjectileList* projectiles, uint32_t numProjectile, Player* player)
{
	double velocity = 0.05;
	// First, see if space key pressed
	uint8_t* keys = SDL_GetKeyboardState(NULL);
	// Hasn't yet fired
	if (keys[SDL_SCANCODE_SPACE] && !player->spacePressed)
	{
		for (int i = 0; i < numProjectile; i++)
		{
			if (projectiles->projectiles[i].state == 0)
			{
				GameEngine_moveEntity(&projectiles->projectiles[i], player->x - 0.1 * sin(player->angle), player->y + 0.1 * cos(player->angle), -0.1);
				projectiles->projectiles[i].angle = player->angle;
				projectiles->projectiles[i].state = 1;
				projectiles->projectiles[i].sprite.frameNum = 1;
				player->spacePressed = 1;
				break;
			}
		}
	}
	else if (!keys[SDL_SCANCODE_SPACE])
	{
		player->spacePressed = 0;
	}
	//printf("spacePressed: %d\n", player->spacePressed);
	// Update bullets
	for (int i = 0; i < numProjectile; i++)
	{
		if (projectiles->projectiles[i].state == 1)
		{
			projectiles->projectiles[i].x += velocity * cos(projectiles->projectiles[i].angle);
			projectiles->projectiles[i].y += velocity * sin(projectiles->projectiles[i].angle);
			if ((projectiles->projectiles[i].angle - player->angle > M_PI/2) || (projectiles->projectiles[i].angle - player->angle < -M_PI/2))
			{
				projectiles->projectiles[i].sprite.frameNum = 1;
			}
			else
			{
				projectiles->projectiles[i].sprite.frameNum = 1;
			}
			if (sqrt((projectiles->projectiles[i].x - player->x)*(projectiles->projectiles[i].x - player->x) + (projectiles->projectiles[i].y - player->y)*(projectiles->projectiles[i].y - player->y)) > player->camera.dist)
			{
				projectiles->projectiles[i].state = 0;
				projectiles->projectiles[i].x = -1000;
				projectiles->projectiles[i].y = -1000;
			}
			GameEngine_updateEntity(&projectiles->projectiles[i]);
		}
	}
}