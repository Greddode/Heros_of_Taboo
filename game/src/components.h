#ifndef COMPONENTS_H
#define COMPONENTS_H

#include "raylib.h"
#include <stdbool.h>
#include "game_types.h"
#include "data/ability_data.h"

// Bitmask flags — one bit per component
#define COMP_POSITION       (1u << 0)
#define COMP_STATS          (1u << 1)
#define COMP_SPRITE_ANIM    (1u << 2)
#define COMP_FALLBACK_COLOR (1u << 3)
#define COMP_AI             (1u << 4)
#define COMP_PICKUP         (1u << 5)
#define COMP_NAME           (1u << 6)
#define COMP_PLAYER_TAG     (1u << 7)
#define COMP_HIT_FLASH      (1u << 8)
#define COMP_ABILITIES      (1u << 9)

// --- Component structs ---

typedef struct {
    int x, y;
    int prevX, prevY;
    Direction facingDir;
} CPosition;

typedef struct {
    int hp, maxHp;
    int attack, defense;
    int level, expValue;
    int str, dex, intel, con, lck;
    int statPoints;
    bool alive;
    // Player-only exp fields (ignored on monsters):
    int exp, expToNext;
} CStats;

typedef struct {
    Texture2D* tex;     // pointer into ResourceManager cache (NOT owned here)
    int row;            // spritesheet row
    int frame;          // current frame
    int frameCount;     // total horizontal frames (0 = static)
    float animTimer;
    float animSpeed;    // seconds per frame
} CSpriteAnim;

typedef struct {
    Color color;
} CFallbackColor;

typedef struct {
    MonsterType type;
    AttackType  attackType;
    int detectionRange;
    int attackRange;
    int lastSeenX, lastSeenY;
    int huntTurns;
    int shadowTurnCounter;
} CAI;

typedef struct {
    bool isEquip;
    ItemType  itemType;     // valid when !isEquip
    EquipType equipType;    // valid when isEquip
    int quantity;
} CPickup;

typedef struct {
    char name[32];
} CName;

typedef struct {
    float timer;   // seconds remaining; entity flashes white while > 0
} CHitFlash;

#define MAX_ABILITIES 8

typedef struct {
    AbilityType abilities[MAX_ABILITIES];
    int         cooldowns[MAX_ABILITIES];
    int         count;
    int         mp;
    int         maxMp;
} CAbilities;

#endif
