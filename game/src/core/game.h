#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <stdbool.h>
#include <math.h>

#define MOVE_ANIM_DURATION 0.15f
#define FOG_RADIUS 7

#include "tmx/tmx.h"
#include "entity/entity.h"
#include "entity/player.h"
#include "data/monster_data.h"
#include "ui/combat_log.h"
#include "inventory.h"
#include "renderer.h"
#include "map_helpers.h"
#include "world.h"

float GetUIScale(void);
void SetGuiScale(float scale);
float GetGuiScale(void);

typedef struct Game {
    MapData* map;
    // tilesetTextures moved to GameWorld

    int turnCount;
    GameState state;

    Camera2D camera;

    unsigned char visibility[MAP_HEIGHT][MAP_WIDTH];
    unsigned char blocking[MAP_HEIGHT][MAP_WIDTH];

    CombatLog combatLog;

    // Legacy inventory and UI state moved to InventoryUIState and GameWorld

    int currentFloor;
    int maxFloors;
    int stairX;
    int stairY;
    bool floorClearedNotified;
    bool escapeSpawned;

    int timeWaited;
    // selectedEntity moved to GameWorld

    int escapeX;
    int escapeY;

    float enemyTurnCooldown;
    float animTimer;
    float monsterAnimTimer;
    float animDuration;
    float monsterAnimDuration;
    bool sprintBypassRoom;
    bool animatingEnemyTurn;

    Projectile projectile;
    float projectileTimer;
    float projectileDuration;

    float levelUpTimer;

    // ECS world (pickups, monsters when migrated, player entity)
    GameWorld ecsWorld;
} Game;

void SyncGameWorldFromGame(Game* game);
bool InitGame(Game* game, const char* tmxFile);
void CleanupGame(Game* game);
void HandleInput(Game* game);
void UpdateGame(Game* game);
void DescendFloor(Game* game);

#endif
