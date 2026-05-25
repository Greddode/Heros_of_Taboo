#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "tmx/tmx.h"
#include "entity/entity.h"
#include "entity/player.h"
#include "ui/combat_log.h"
#include <stdbool.h>

#define MAX_HEALING 32
#define MAP_WIDTH 100
#define MAP_HEIGHT 100
#define MOVE_ANIM_DURATION 0.15f
#define FOG_RADIUS 7

// Turn / state machine
typedef enum {
    STATE_PLAYER_TURN,
    STATE_ENEMY_TURN,
    STATE_GAME_OVER,
    STATE_WIN
} GameState;

typedef struct Game {
    MapData* map;
    Texture2D tilesetTextures[MAX_TILESETS];

    Player player;

    int turnCount;
    GameState state;

    Camera2D camera;

    unsigned char visibility[MAP_HEIGHT][MAP_WIDTH];
    unsigned char blocking[MAP_HEIGHT][MAP_WIDTH];

    CombatLog combatLog;

    int healingCount;
    int healingTiles[MAX_HEALING][2];
    bool healingCollected[MAX_HEALING];

    float enemyTurnCooldown;
    float animTimer;
    float monsterAnimTimer;
} Game;

bool InitGame(Game* game, const char* tmxFile);
void CleanupGame(Game* game);
void HandleInput(Game* game);
void UpdateGame(Game* game);
void RenderGame(const Game* game);

#endif