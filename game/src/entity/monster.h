#ifndef MONSTER_H
#define MONSTER_H

#include "game_types.h"
#include "raylib.h"
#include <stdbool.h>

#define MAX_MONSTERS 64
#define MONSTER_NAME_LEN 32

// -- Template: shared definition for all monsters of a given type ------------
typedef struct {
    MonsterType type;
    const char* tmxTypeName;
    char name[MONSTER_NAME_LEN];
    int hp;
    int attack;
    int defense;
    int expValue;
    int level;
    int str, dex, intel, con, lck;
    Color color;
    const char* spritePath;
    int frameCount;
    float animSpeed;
    int detectionRange;
    int minFloor;
    int spawnWeight;
    AttackType attackType;
    int attackRange;
} MonsterTemplate;

// Call once at game startup to register all monster type templates.
void Monster_InitTemplates(void);

// Retrieve the template for a given type.
const MonsterTemplate* Monster_GetTemplate(MonsterType type);

// Legacy runtime API — used by old code during transition, will be removed
// when all callers are migrated to ECS (Step 10).
typedef struct {
    MonsterType type;
    char name[MONSTER_NAME_LEN];
    int x, y;
    int prevX, prevY;
    int hp;
    int maxHp;
    int attack;
    int defense;
    int level;
    int expValue;
    int str, dex, intel, con, lck;
    bool alive;
    bool active;
    bool facingRight;
    int animFrame;
    float animTimer;
    float hitFlashTimer;
    int shadowTurnCounter;
    AttackType attackType;
    int attackRange;
    int lastSeenX, lastSeenY;
    int huntTurns;
} Monster;

Monster* Monster_Spawn(MonsterType type, int x, int y, int floor);
Monster* Monster_SpawnByTypeName(const char* tmxTypeName, int x, int y, int floor);
void Monster_ResetAll(void);
int Monster_GetAliveCount(void);
bool Monster_AreAllDead(void);
Monster* Monster_GetAt(int x, int y, const Monster* exclude);
Monster* Monster_GetArray(void);
int Monster_GetCount(void);
void Monster_UpdateAnimations(float dt);
void Monster_LoadSprites(void);
void Monster_UnloadSprites(void);

typedef struct Game Game;
bool Monster_ProcessAllAI(Game* game, int timeWaited);
void Monster_RenderAll(const unsigned char visibility[][MAP_WIDTH],
                        int mapWidth, int mapHeight,
                        int tileWidth, int tileHeight,
                        float t);

#endif
