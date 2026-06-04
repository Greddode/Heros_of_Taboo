#ifndef WORLD_H
#define WORLD_H

#include "raylib.h"
#include "ecs.h"
#include "map/tmx/tmx.h"
#include "game_types.h"
#include "inventory.h"
#include "data/biome_data.h"

#define MAX_DAMAGE_NUMBERS 32
#define DAMAGE_NUMBER_LIFETIME 0.8f
#define DAMAGE_NUMBER_FLOAT_SPEED 20.0f

#define MAX_FLOAT_MSGS 16
#define FLOAT_MSG_LIFETIME 1.5f
#define FLOAT_MSG_VEL_Y -30.0f

typedef struct {
    bool active;
    char text[64];
    float worldX, worldY;
    float velY;
    float alpha;
    float lifetime;
    Color color;
} FloatMsg;

typedef struct {
    FloatMsg entries[MAX_FLOAT_MSGS];
} FloatMsgPool;

typedef struct {
    bool active;
    char text[16];
    Vector2 pos;          // world-space position
    float timer;
    Color color;
} DamageNumber;

typedef struct {
    DamageNumber entries[MAX_DAMAGE_NUMBERS];
} DamageNumberPool;

typedef struct GameWorld {
    // ECS
    World ecs;
    EntityId playerEntity;

    // Performance: monster spatial hash grid (tile → entity lookup)
    EntityId monsterGrid[MAP_HEIGHT][MAP_WIDTH];
    int aliveMonsterCount;
    int gold;
    bool statCapsRemoved;
    BiomeType currentBiome;
    EntityId shopkeeperEntity;

    // Floating damage numbers
    DamageNumberPool damageNumbers;
    // Floating status messages
    FloatMsgPool floatMsgs;

    // Map
    MapData* map;
    // Pointers into ResourceManager cache (owned by resources.c).
    // fallbackTileset is owned locally for generated checkerboard fallback.
    Texture2D* tilesetTextures[MAX_TILESETS];
    Texture2D fallbackTileset;

    // Turn-based state machine
    GameState state;
    int turnCount;
    int timeWaited;

    // Floor
    int currentFloor;
    int maxFloors;
    int stairX, stairY;
    int escapeX, escapeY;
    bool floorClearedNotified;
    bool escapeSpawned;
    bool monstersEverSpawned;
    bool shadowSpawned;

    // Animation
    float animTimer, animDuration;
    float monsterAnimTimer, monsterAnimDuration;
    float enemyTurnCooldown;
    bool  animatingEnemyTurn;
    bool  sprintBypassRoom;

    // Projectile
    Projectile projectile;
    float projectileTimer, projectileDuration;

    // Fog of war
    unsigned char visibility[MAP_HEIGHT][MAP_WIDTH];
    unsigned char blocking[MAP_HEIGHT][MAP_WIDTH];

    // Camera
    Camera2D camera;

    // Level-up flash
    float levelUpTimer;

    // Inspector (ECS entity id, ENTITY_NONE if none)
    EntityId selectedMonsterEntity;

    // Tile inspector (selected tile for potion/equipment info panel)
    int inspectedTileX;
    int inspectedTileY;
    bool inspectedTileActive;

    // Inventory data (UI state will be extracted later)
    InventorySlot inventory[MAX_INVENTORY_SLOTS];
    int inventorySlotCount;
    EquipType equipped[EQUIP_SLOT_COUNT];
    EquipType equipInventory[MAX_INVENTORY_SLOTS];
    int equipInventoryCount;

    // Minimap
    bool mapOpen;
} GameWorld;

void GameWorld_Init(GameWorld* gw);

void DamageNumber_Spawn(DamageNumberPool* pool, int value, int tileX, int tileY, int tw, int th, Color color);
void DamageNumber_UpdateAll(DamageNumberPool* pool, float dt);

void FloatMsg_Spawn(GameWorld* gw, int tileX, int tileY, Color color, const char* fmt, ...);
void FloatMsg_UpdateAll(GameWorld* gw, float dt);

#endif
