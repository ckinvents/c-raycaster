#ifndef GAMEENGINE_H
#define GAMEENGINE_H

#include "rayengine.h"
//#include <SDL2/SDL_mixer.h>

typedef struct _Player {
	double x;
	double y;
    double h;
    double groundH;
    double velX;
    double velY;
    double velH;
	double angle;
    uint8_t usingMouse;
    uint8_t health;
    uint8_t state;
    double timer;
    uint8_t spacePressed;
    Camera camera;
    //Mix_Chunk* walkSound;
} Player;

enum KeyMapNames {
    PK_FORWARD,
    PK_BACKWARD,
    PK_LSTRAFE,
    PK_RSTRAFE,
    PK_TC,
    PK_TCC,
    PK_JUMP,
    PK_CROUCH,
    PK_SPRINT,
    PK_PAUSE,
    PK_KILL,
    PK_RESPAWN,
    PK_USE,
    PK_INTERACT,
    LEN_PK,
    TERMINATE_PK
};

typedef struct {
    uint8_t primary;
    uint8_t secondary;
} KeyPair;

typedef struct {
    KeyPair keys[LEN_PK];
    uint8_t state[LEN_PK];
} KeyMap;

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

typedef struct {
    Entity projectiles[64];
} ProjectileList;

void GameEngine_initPlayer(Player* player, double x, double y, double angle, uint8_t usingMouse, double fov, double viewDist, uint32_t screenWidth);
void GameEngine_initEntity(Entity* entity, double x, double y, double h, double angle, RayTex* spriteTex, RayTex* shadowTex);
void GameEngine_initProjectiles(ProjectileList* projectiles, uint32_t numProjectile, RayTex* spriteTex, RayTex* shadowTex);
void GameEngine_moveEntity(Entity* entity, double x, double y, double h);
void GameEngine_scaleEntity(Entity* entity, double scaleFactor);
void GameEngine_updatePlayer(Player* player, Map* map, KeyMap* keyMap, double dt);
void GameEngine_updateEntity(Entity* entity);
void GameEngine_updateProjectile(ProjectileList* projectiles, uint32_t numProjectile, Player* player);
void GameEngine_bindKeys(KeyMap* keyMap, uint8_t* keyList);
void GameEngine_updateKeys(KeyMap* keyMap);

#endif //GAMEENGINE_H