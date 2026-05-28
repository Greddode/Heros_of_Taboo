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
    Texture2D tilesetTextures[MAX_TILESETS];

    Player player;

    int turnCount;
    GameState state;

    Camera2D camera;

    unsigned char visibility[MAP_HEIGHT][MAP_WIDTH];
    unsigned char blocking[MAP_HEIGHT][MAP_WIDTH];

    CombatLog combatLog;

    int selectedPotionTileX;
    int selectedPotionTileY;
    bool selectedPotionTileActive;

    Texture2D potionTextures[3];
    Texture2D texLoot;
    Texture2D texUiFrame;
    Texture2D texUiSlot;
    Texture2D texUiMarker;
    Texture2D magicAttacksTexture;

    InventorySlot inventory[MAX_INVENTORY_SLOTS];
    int inventorySlotCount;
    int inventorySelection;
    int invScrollOffset;
    int statsScrollCol1;
    int statsScrollCol2;
    int statsActiveCol;
    int statsSelection;
    InventorySubState invSubState;
    int invActionSelection;
    InventoryTab inventoryTab;

    EquipType equipped[EQUIP_SLOT_COUNT];
    EquipType equipInventory[MAX_INVENTORY_SLOTS];
    int equipInventoryCount;

    int currentFloor;
    int maxFloors;
    int stairX;
    int stairY;
    bool floorClearedNotified;
    bool escapeSpawned;

    int timeWaited;
    EntityId selectedMonsterEntity;

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
