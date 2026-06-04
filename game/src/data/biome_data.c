#include "biome_data.h"
#include <stddef.h>

static const BiomeDef BIOME_TABLE[BIOME_COUNT] = {
    { BIOME_NONE, NULL, 0, 0, 0, { 0 }, 0, NULL },
    {
        .id            = BIOME_GOBLIN_DEN,
        .name          = "Goblin Den",
        .minFloor      = 1,
        .maxFloor      = 5,
        .spawnWeight   = 10,
        .monsterPool   = { MONSTER_GOBLIN },
        .monsterCount  = 1,
        .tilesetPath   = "resources/tilesets/dungeon.png",
    },
};

const BiomeDef* GetBiomeData(BiomeType type)
{
    if (type < 0 || type >= BIOME_COUNT) return NULL;
    return &BIOME_TABLE[type];
}

BiomeType Biome_SelectForFloor(int floorNumber)
{
    BiomeType candidates[BIOME_COUNT];
    float   weights[BIOME_COUNT];
    int     candidateCount = 0;
    float   totalWeight = 0.0f;

    for (int i = 1; i < BIOME_COUNT; i++) {
        const BiomeDef* b = &BIOME_TABLE[i];
        if (floorNumber >= b->minFloor && floorNumber <= b->maxFloor) {
            candidates[candidateCount] = (BiomeType)i;
            weights[candidateCount] = (float)b->spawnWeight;
            totalWeight += weights[candidateCount];
            candidateCount++;
        }
    }

    if (candidateCount == 0) return BIOME_NONE;

    float roll = (float)(GetRandomValue(0, (int)(totalWeight * 100))) / 100.0f;
    float acc = 0.0f;
    for (int i = 0; i < candidateCount; i++) {
        acc += weights[i];
        if (roll <= acc) return candidates[i];
    }
    return candidates[candidateCount - 1];
}
