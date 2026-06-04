#ifndef BIOME_DATA_H
#define BIOME_DATA_H

#include "game_types.h"

#define BIOME_MONSTER_POOL_MAX 16

typedef enum { BIOME_NONE = 0, BIOME_GOBLIN_DEN, BIOME_COUNT } BiomeType;

typedef struct {
    BiomeType   id;
    const char* name;
    int         minFloor;
    int         maxFloor;
    int         spawnWeight;
    MonsterType monsterPool[BIOME_MONSTER_POOL_MAX];
    int         monsterCount;
    const char* tilesetPath;
} BiomeDef;

const BiomeDef* GetBiomeData(BiomeType type);
BiomeType       Biome_SelectForFloor(int floorNumber);

#endif
