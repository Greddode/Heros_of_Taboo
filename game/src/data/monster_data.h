#ifndef MONSTER_DATA_H
#define MONSTER_DATA_H

#include "game_types.h"
#include "raylib.h"
#include <stdbool.h>

#define MONSTER_NAME_LEN 32

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
    int maxFloor;      // inclusive; -1 means no cap
    int maxLevel;      // stat scaling hard cap; -1 means no cap
    int spawnWeight;
    AttackType attackType;
    int attackRange;
} MonsterTemplate;

void Monster_InitTemplates(void);
const MonsterTemplate* Monster_GetTemplate(MonsterType type);
// Returns MONSTER_TYPE_COUNT if no match.
MonsterType Monster_FindTypeByTmxName(const char* tmxTypeName);

#endif
