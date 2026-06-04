#include "monster_data.h"
#include "game_balance.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static MonsterTemplate s_templates[MONSTER_TYPE_COUNT];
static bool s_templatesReady = false;

void Monster_InitTemplates(void) {
    if (s_templatesReady) return;
    s_templatesReady = true;

    s_templates[MONSTER_SHADOW] = (MonsterTemplate){
        .type            = MONSTER_SHADOW,
        .tmxTypeName     = "shadow",
        .name            = "Shadow",
        .hp              = 10,
        .attack          = 4,
        .defense         = 1,
        .expValue        = 10,
        .level           = 1,
        .str             = 4,
        .dex             = 6,
        .intel           = 2,
        .con             = 3,
        .lck             = 5,
        .color           = { 40, 0, 80, 255 },
        .spritePath      = "resources/sprites/monsters/Shadow.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 20,
        .minFloor        = 1,
        .maxFloor        = -1,
        .maxLevel        = -1,
        .spawnWeight     = 0,
        .attackType      = ATTACK_MELEE,
        .attackRange     = 1,
    };

    // --- Group 1: Early (floors 1+) -----------------------------------------
    s_templates[MONSTER_BAT] = (MonsterTemplate){
        .type            = MONSTER_BAT,
        .tmxTypeName     = "bat",
        .name            = "Bat",
        .hp              = 5,
        .attack          = 2,
        .defense         = 0,
        .expValue        = 6,
        .level           = 1,
        .str             = 1,
        .dex             = 3,
        .intel           = 0,
        .con             = 1,
        .lck             = 1,
        .color           = { 100, 60, 140, 255 },
        .spritePath      = "resources/sprites/monsters/Bat.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 1,
        .maxFloor        = 3,
        .maxLevel        = 3,
        .spawnWeight     = 12,
        .attackType      = ATTACK_MELEE,
        .attackRange     = 1,
    };

    s_templates[MONSTER_GOBLIN] = (MonsterTemplate){
        .type            = MONSTER_GOBLIN,
        .tmxTypeName     = "goblin",
        .name            = "Goblin",
        .hp              = 7,
        .attack          = 3,
        .defense         = 1,
        .expValue        = 8,
        .level           = 1,
        .str             = 2,
        .dex             = 2,
        .intel           = 1,
        .con             = 2,
        .lck             = 2,
        .color           = { 80, 160, 80, 255 },
        .spritePath      = "resources/sprites/monsters/Goblin.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 1,
        .maxFloor        = 4,
        .maxLevel        = 4,
        .spawnWeight     = 13,
        .attackType      = ATTACK_MELEE,
        .attackRange     = 1,
        .weaponPool      = { EQUIP_NONE, EQUIP_SURVIVAL_KNIFE, EQUIP_SIMPLE_BOW },
        .weaponPoolCount = 3,
        .armorPool       = { EQUIP_NONE },
        .armorPoolCount  = 1,
        .equipDropChance = 25,
    };

    s_templates[MONSTER_SKELETON] = (MonsterTemplate){
        .type            = MONSTER_SKELETON,
        .tmxTypeName     = "skeleton",
        .name            = "Skeleton",
        .hp              = 8,
        .attack          = 3,
        .defense         = 1,
        .expValue        = 10,
        .level           = 1,
        .str             = 2,
        .dex             = 1,
        .intel           = 0,
        .con             = 3,
        .lck             = 1,
        .color           = { 200, 200, 180, 255 },
        .spritePath      = "resources/sprites/monsters/Skeleton.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 1,
        .maxFloor        = 5,
        .maxLevel        = 5,
        .spawnWeight     = 11,
        .attackType      = ATTACK_MELEE,
        .attackRange     = 1,
    };

    // --- Group 2: Mid (floors 3+) -------------------------------------------
    s_templates[MONSTER_FLOATING_EYE] = (MonsterTemplate){
        .type            = MONSTER_FLOATING_EYE,
        .tmxTypeName     = "floating_eye",
        .name            = "Floating Eye",
        .hp              = 8,
        .attack          = 4,
        .defense         = 1,
        .expValue        = 18,
        .level           = 1,
        .str             = 3,
        .dex             = 2,
        .intel           = 5,
        .con             = 3,
        .lck             = 2,
        .color           = { 50, 220, 80, 255 },
        .spritePath      = "resources/sprites/monsters/Floating_Eye.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 3,
        .maxFloor        = -1,
        .maxLevel        = -1,
        .spawnWeight     = 8,
        .attackType      = ATTACK_MAGIC,
        .attackRange     = 4,
    };

    s_templates[MONSTER_FUNGAL_MYCONID] = (MonsterTemplate){
        .type            = MONSTER_FUNGAL_MYCONID,
        .tmxTypeName     = "fungal_myconid",
        .name            = "Fungal Myconid",
        .hp              = 9,
        .attack          = 5,
        .defense         = 2,
        .expValue        = 20,
        .level           = 1,
        .str             = 2,
        .dex             = 4,
        .intel           = 0,
        .con             = 5,
        .lck             = 1,
        .color           = { 220, 220, 200, 255 },
        .spritePath      = "resources/sprites/monsters/Fungal_Myconid.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 3,
        .maxFloor        = -1,
        .maxLevel        = 8,
        .spawnWeight     = 8,
        .attackType      = ATTACK_RANGED,
        .attackRange     = 3,
    };

    s_templates[MONSTER_WARP_SKULL] = (MonsterTemplate){
        .type            = MONSTER_WARP_SKULL,
        .tmxTypeName     = "warp_skull",
        .name            = "Warp Skull",
        .hp              = 11,
        .attack          = 5,
        .defense         = 1,
        .expValue        = 24,
        .level           = 1,
        .str             = 1,
        .dex             = 3,
        .intel           = 5,
        .con             = 2,
        .lck             = 2,
        .color           = { 160, 40, 200, 255 },
        .spritePath      = "resources/sprites/monsters/Warp_Skull.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 10,
        .minFloor        = 3,
        .maxFloor        = -1,
        .maxLevel        = 8,
        .spawnWeight     = 7,
        .attackType      = ATTACK_MAGIC,
        .attackRange     = 3,
    };

    // --- Group 3: Late (floors 6+) ------------------------------------------
    s_templates[MONSTER_DEMON_EYE] = (MonsterTemplate){
        .type            = MONSTER_DEMON_EYE,
        .tmxTypeName     = "demon_eye",
        .name            = "Demon Eye",
        .hp              = 14,
        .attack          = 6,
        .defense         = 2,
        .expValue        = 45,
        .level           = 1,
        .str             = 5,
        .dex             = 2,
        .intel           = 6,
        .con             = 4,
        .lck             = 2,
        .color           = { 200, 50, 50, 255 },
        .spritePath      = "resources/sprites/monsters/Demon_Eye.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 10,
        .minFloor        = 6,
        .maxFloor        = -1,
        .maxLevel        = -1,
        .spawnWeight     = 6,
        .attackType      = ATTACK_MAGIC,
        .attackRange     = 5,
    };

    s_templates[MONSTER_OGRE] = (MonsterTemplate){
        .type            = MONSTER_OGRE,
        .tmxTypeName     = "ogre",
        .name            = "Ogre",
        .hp              = 22,
        .attack          = 6,
        .defense         = 3,
        .expValue        = 55,
        .level           = 1,
        .str             = 7,
        .dex             = 1,
        .intel           = 0,
        .con             = 7,
        .lck             = 1,
        .color           = { 120, 60, 180, 255 },
        .spritePath      = "resources/sprites/monsters/Ogre.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 8,
        .minFloor        = 6,
        .maxFloor        = -1,
        .maxLevel        = 12,
        .spawnWeight     = 8,
        .attackType      = ATTACK_MELEE,
        .attackRange     = 1,
    };

    s_templates[MONSTER_DRAGON] = (MonsterTemplate){
        .type            = MONSTER_DRAGON,
        .tmxTypeName     = "dragon",
        .name            = "Dragon",
        .hp              = 22,
        .attack          = 7,
        .defense         = 3,
        .expValue        = 70,
        .level           = 1,
        .str             = 9,
        .dex             = 6,
        .intel           = 4,
        .con             = 8,
        .lck             = 3,
        .color           = { 180, 80, 20, 255 },
        .spritePath      = "resources/sprites/monsters/Dragon.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 10,
        .minFloor        = 6,
        .maxFloor        = -1,
        .maxLevel        = -1,
        .spawnWeight     = 5,
        .attackType      = ATTACK_RANGED,
        .attackRange     = 6,
    };

    // --- Ranger: early floor 2+ (shorter hp, ranged attack) -------------------
    s_templates[MONSTER_RANGER_GOBLIN] = (MonsterTemplate){
        .type            = MONSTER_RANGER_GOBLIN,
        .tmxTypeName     = "goblin_archer",
        .name            = "Goblin Archer",
        .hp              = 5,
        .attack          = 2,
        .defense         = 0,
        .expValue        = 11,
        .level           = 1,
        .str             = 1,
        .dex             = 5,
        .intel           = 1,
        .con             = 1,
        .lck             = 3,
        .color           = { 100, 150, 60, 255 },
        .spritePath      = "resources/sprites/monsters/Goblin_Archer.png",
        .frameCount      = 4,
        .animSpeed       = 0.5f,
        .detectionRange  = 9,
        .minFloor        = 2,
        .maxFloor        = 5,
        .maxLevel        = 5,
        .spawnWeight     = 9,
        .attackType      = ATTACK_RANGED,
        .attackRange     = 5,
    };
}

const MonsterTemplate* Monster_GetTemplate(MonsterType type) {
    if (!s_templatesReady) Monster_InitTemplates();
    if (type < 0 || type >= MONSTER_TYPE_COUNT) return NULL;
    return &s_templates[type];
}

MonsterType Monster_FindTypeByTmxName(const char* tmxTypeName) {
    if (!tmxTypeName) return MONSTER_TYPE_COUNT;
    if (!s_templatesReady) Monster_InitTemplates();
    for (int i = 0; i < MONSTER_TYPE_COUNT; i++) {
        const MonsterTemplate* tpl = &s_templates[i];
        if (tpl->tmxTypeName && strcmp(tmxTypeName, tpl->tmxTypeName) == 0)
            return (MonsterType)i;
    }
    return MONSTER_TYPE_COUNT;
}

float Monster_SnapCRToTier(float raw) {
    static const float tiers[] = {
        0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 3.0f,
        4.0f, 5.0f, 6.0f, 8.0f, 10.0f, 12.0f, 15.0f, 20.0f
    };
    static const int count = 14;
    float best = tiers[0];
    float bestDiff = fabsf(raw - tiers[0]);
    for (int i = 1; i < count; i++) {
        float diff = fabsf(raw - tiers[i]);
        if (diff < bestDiff) { bestDiff = diff; best = tiers[i]; }
    }
    return best;
}

float Monster_CalcCR(const MonsterTemplate* def, int currentFloor) {
    if (!def) return 0.0f;

    int effectiveHp  = def->hp + (def->con * CON_HP_SCALE);
    int effectiveDef = def->defense;
    float defensiveScore = (float)effectiveHp + (float)effectiveDef * 3.0f;

    float baseDamage = (float)def->attack + (float)(def->str * STR_ATTACK_SCALE);
    float critMod    = 1.0f + (float)def->lck / 200.0f;
    float offensiveScore = baseDamage * critMod * 4.0f;

    float rawCR = (defensiveScore / 20.0f + offensiveScore / 15.0f) / 2.0f;

    float floorScale = 1.0f + (float)(currentFloor - def->minFloor) * 0.12f;
    if (floorScale < 1.0f) floorScale = 1.0f;

    return Monster_SnapCRToTier(rawCR * floorScale);
}

float Monster_GetFloorBudget(int floorNumber) {
    return FLOOR_DR_BASE + (float)(floorNumber - 1) * FLOOR_DR_PER_FLOOR;
}
