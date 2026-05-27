#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include <stdbool.h>
#include <math.h>

#define MAP_WIDTH 100
#define MAP_HEIGHT 100
#define MOVE_ANIM_DURATION 0.15f
#define FOG_RADIUS 7
#define PROJECTILE_ANIM_DURATION 0.25f

#include "tmx/tmx.h"
#include "entity/entity.h"
#include "entity/player.h"
#include "entity/monster.h"
#include "ui/combat_log.h"
#include "inventory.h"
#include "renderer.h"
#include "map_helpers.h"

float GetUIScale(void);
void SetGuiScale(float scale);
float GetGuiScale(void);

typedef enum {
    STATE_PLAYER_TURN,
    STATE_ENEMY_TURN,
    STATE_GAME_OVER,
    STATE_WIN,
    STATE_INVENTORY
} GameState;

typedef struct {
    bool active;
    float sx, sy;    // start pixel coords (world space)
    float ex, ey;    // end pixel coords (world space)
    Color color;
    int tileSX, tileSY;  // caster tile coords
    int tileEX, tileEY;  // target tile coords
    AttackType attackType;
    int startFrame;       // first frame index (0-based) in magic texture
    int animFrameCount;   // number of animation frames
} Projectile;

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
    int potionQuantities[MAX_POTIONS];
    int selectedPotionTileX;
    int selectedPotionTileY;
    bool selectedPotionTileActive;

    Texture2D potionTextures[3];
    Texture2D texUiFrame;
    Texture2D texUiSlot;
    Texture2D magicAttacksTexture;

    InventorySlot inventory[MAX_INVENTORY_SLOTS];
    int inventorySlotCount;
    int inventorySelection;
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
    int selectedMonsterIdx;

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
} Game;

bool InitGame(Game* game, const char* tmxFile);
void CleanupGame(Game* game);
void HandleInput(Game* game);
void UpdateGame(Game* game);
void DescendFloor(Game* game);

#endif
