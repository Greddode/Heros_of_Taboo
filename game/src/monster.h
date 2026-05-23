#ifndef MONSTER_H
#define MONSTER_H

#include "raylib.h"
#include <stdbool.h>

#define MAX_MONSTERS 64
#define MONSTER_NAME_LEN 32

// ============================================================================
// Monster System — expandable, data-driven monster definitions
//
// HOW TO ADD A NEW MONSTER TYPE:
//   1. Add an entry to the MonsterType enum (before MONSTER_TYPE_COUNT).
//   2. Add a MonsterTemplate entry in Monster_InitTemplates() in monster.c.
//   3. (Optional) Place a sprite sheet in resources/sprite_animations/idle/
//      and set spritePath / frameCount in the new template.
// No changes to game.c needed — Monster_SpawnByTypeName() auto-maps TMX objects.
// ============================================================================

// -- Monster type identifiers ------------------------------------------------
typedef enum {
    MONSTER_FLOATING_EYE,
    MONSTER_FUNGAL_MYCONID,
    MONSTER_OGRE,
    MONSTER_TYPE_COUNT
} MonsterType;

// -- Template: shared definition for all monsters of a given type ------------
typedef struct {
    MonsterType type;
    const char* tmxTypeName;       // matches TMX object type field
    char name[MONSTER_NAME_LEN];
    int hp;
    int attack;
    int defense;
    int expValue;
    int level;
    Color color;               // fallback colour when no sprite is available
    const char* spritePath;    // path to PNG sprite sheet, or NULL
    int frameCount;            // horizontal frames in the sheet (0 = static)
    float animSpeed;           // seconds per animation frame
    int detectionRange;        // tiles within which the monster notices the player
} MonsterTemplate;

// -- Instance: runtime data for a single monster on the map ------------------
typedef struct {
    MonsterType type;
    char name[MONSTER_NAME_LEN];
    int x, y;              // current tile position
    int prevX, prevY;      // previous tile position (for smooth interpolation)
    int hp;
    int maxHp;
    int attack;            // current combat stats (copied from template at spawn)
    int defense;
    int level;
    int expValue;
    bool alive;
    bool active;           // false = empty slot
    bool facingRight;
    int animFrame;
    float animTimer;
    float hitFlashTimer;
} Monster;

// ============================================================================
//  PUBLIC API
// ============================================================================

// Call once at game startup to register all monster type templates.
void Monster_InitTemplates(void);

// Retrieve the template for a given type.
const MonsterTemplate* Monster_GetTemplate(MonsterType type);

// Spawn a monster at (x, y). Returns a pointer or NULL if the pool is full.
Monster* Monster_Spawn(MonsterType type, int x, int y);

// Spawn a monster by its TMX object type name (matches tmxTypeName exactly).
// Returns NULL if no template matches or the pool is full.
Monster* Monster_SpawnByTypeName(const char* tmxTypeName, int x, int y);

// Destroy all monsters (for restarting the game).
void Monster_ResetAll(void);

// Number of monsters still alive.
int Monster_GetAliveCount(void);

// Returns true if every monster is dead.
bool Monster_AreAllDead(void);

// Find a living monster at (x, y), excluding the given pointer (or NULL).
Monster* Monster_GetAt(int x, int y, const Monster* exclude);

// Direct access to the internal array and total slot count.
Monster* Monster_GetArray(void);
int Monster_GetCount(void);

// Advance animation timers (call every frame).
void Monster_UpdateAnimations(float dt);

// Run AI for all living monsters.
//  * playerX, playerY, playerDefense, playerHp — the player's current state
//  * playerHitFlash — the player's hitFlashTimer (set on attack for white flash)
//  * blocking — the walkability grid (1 = blocked, 0 = open)
//  * mapWidth, mapHeight — dimensions of the blocking grid
// The function applies damage to *playerHp when monsters attack.
// Returns false if the player was killed.
bool Monster_ProcessAllAI(int playerX, int playerY, int* playerHp, int playerDefense,
                           float* playerHitFlash,
                           const unsigned char blocking[][100],
                           int mapWidth, int mapHeight);

// Load shared monster sprite sheets (call after InitGame map loading).
void Monster_LoadSprites(void);

// Unload shared monster sprite sheets.
void Monster_UnloadSprites(void);

// Render all visible monsters.
//  * t — interpolation factor (0.0 = previous positions, 1.0 = current).
//    Pass -1.0 or a value outside [0,1] to skip interpolation.
void Monster_RenderAll(const unsigned char visibility[][100],
                        int mapWidth, int mapHeight,
                        int tileWidth, int tileHeight,
                        float t);

#endif
