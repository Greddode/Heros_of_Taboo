#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "tmx/tmx.h"
#include "entity/entity.h"
#include "entity/player.h"
#include "ui/combat_log.h"
#include <stdbool.h>

#define MAX_POTIONS 32
#define MAP_WIDTH 100
#define MAP_HEIGHT 100
#define MOVE_ANIM_DURATION 0.15f
#define FOG_RADIUS 7
#define MAX_INVENTORY_SLOTS 16

// Item types for inventory
typedef enum {
    ITEM_NONE = 0,
    ITEM_SMALL_HP_POTION,
    ITEM_BIG_HP_POTION,
    ITEM_LARGE_HP_POTION,
    ITEM_COUNT
} ItemType;

typedef struct {
    ItemType type;
    int quantity;
} InventorySlot;

// Turn / state machine
typedef enum {
    STATE_PLAYER_TURN,
    STATE_ENEMY_TURN,
    STATE_GAME_OVER,
    STATE_WIN,
    STATE_INVENTORY
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

    int potionCount;
    int potionTiles[MAX_POTIONS][2];
    bool potionCollected[MAX_POTIONS];
    ItemType potionTypes[MAX_POTIONS];

    InventorySlot inventory[MAX_INVENTORY_SLOTS];
    int inventorySlotCount;
    int inventorySelection;

    int currentFloor;
    int maxFloors;
    int stairX;
    int stairY;
    bool floorClearedNotified;
    bool escapeSpawned;

    int timeWaited;
    int selectedMonsterIdx;

    int escapeX;
    int escapeY;

    float enemyTurnCooldown;
    float animTimer;
    float monsterAnimTimer;
    float animDuration;
    float monsterAnimDuration;
    bool sprintBypassRoom;
} Game;

bool InitGame(Game* game, const char* tmxFile);
void CleanupGame(Game* game);
void HandleInput(Game* game);
void UpdateGame(Game* game);
void RenderGame(const Game* game);
void DescendFloor(Game* game);

const char* GetItemName(ItemType type);
int GetItemHealAmount(ItemType type);
bool InventoryAdd(Game* game, ItemType type);
bool InventoryUse(Game* game, int slot);

#endif