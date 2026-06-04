#ifndef ABILITY_DATA_H
#define ABILITY_DATA_H

typedef enum {
    ABILITY_NONE = 0,
    ABILITY_PUNCH,
    ABILITY_LIGHT_MELEE,
    ABILITY_HEAVY_MELEE,
    ABILITY_RANGED,
    ABILITY_COUNT
} AbilityType;

typedef struct {
    AbilityType id;
    const char* name;
    const char* description;
    int         mpCost;
    int         cooldownMax;
} AbilityData;

const AbilityData* GetAbilityData(AbilityType type);

#endif
