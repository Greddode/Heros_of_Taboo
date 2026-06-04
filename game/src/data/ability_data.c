#include "ability_data.h"
#include <stddef.h>

static const AbilityData ABILITY_TABLE[ABILITY_COUNT] = {
    { ABILITY_NONE,        "",               "",                           0, 0 },
    { ABILITY_PUNCH,       "Punch",          "A basic unarmed strike.",    0, 0 },
    { ABILITY_LIGHT_MELEE, "Light Strike",   "A quick weapon attack.",     0, 0 },
    { ABILITY_HEAVY_MELEE, "Heavy Strike",   "A powerful weapon attack.",  0, 0 },
    { ABILITY_RANGED,      "Ranged Attack",  "Attack from a distance.",    0, 0 },
};

const AbilityData* GetAbilityData(AbilityType type)
{
    if (type < 0 || type >= ABILITY_COUNT) return NULL;
    return &ABILITY_TABLE[type];
}
