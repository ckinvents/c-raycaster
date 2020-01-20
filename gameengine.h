#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include "rayengine.h"

typedef struct _Player {
	double x;
	double y;
	double angle;
    uint8_t health;
    uint8_t state;
    Camera camera;
} Player;

typedef struct _Entity {
    double x;
    double y;
    double h;
    double angle;
    uint8_t health;
    uint8_t state;
	RaySprite shadow;
	RaySprite sprite;
} Entity;

void GameEngine_initPlayer(Player* player, double x, double y, double angle, double fov, double viewDist, uint32_t screenWidth);
void GameEngine_initEntity(Entity* entity, double x, double y, double h, double angle, RayTex* spriteTex, RayTex* shadowTex);
void GameEngine_moveEntity(Entity* entity, double x, double y, double h);
void GameEngine_scaleEntity(Entity* entity, double scaleFactor);
void GameEngine_updatePlayer(Player* player, Map* map, double dt);
void GameEngine_updateEntity(Entity* entity);

#endif //GAMEENGINE_H