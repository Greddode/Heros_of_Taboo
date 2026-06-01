#ifndef WORLD_H
#define WORLD_H

#include "raylib.h"
#include "ecs.h"
#include "map/tmx/tmx.h"
#include "ui/combat_log.h"
#include "game_types.h"
#include "inventory.h"

typedef struct GameWorld {
    // ECS
    World ecs;
    EntityId playerEntity;

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

    // Combat log
    CombatLog combatLog;

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

#endif
