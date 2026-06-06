#include "spawner_system.h"
#include "debug_log.h"
#include "validation.h"
#include "atlas.h"
#include "event_bus.h"
#include "data/loot_data.h"
#include "data/monster_data.h"
#include "data/biome_data.h"
#include "data/equip_data.h"
#include "world_monster.h"
#include "spatial_hash.h"
#include "resources.h"
#include "inventory.h"
#include "equipment_bonus.h"
#include "game_balance.h"
#include "data/ability_data.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MIN_PLAYER_DIST 5

typedef struct {
    MonsterType type;
    const char* altName;
    int chance;
} MonsterNameVariant;

static const MonsterNameVariant s_nameVariants[] = {
    { MONSTER_BAT,           "Ozzy's Pet",         10 },
    { MONSTER_BAT,           "Definitely a Bird",   8 },
    { MONSTER_GOBLIN,        "Shrekt",              10 },
    { MONSTER_GOBLIN,        "Mike Wazowski",       8 },
    { MONSTER_SKELETON,      "Mr. Bones",          12 },
    { MONSTER_SKELETON,      "Sans",                5 },
    { MONSTER_FLOATING_EYE,  "Big Brother",        10 },
    { MONSTER_FLOATING_EYE,  "Google",              8 },
    { MONSTER_OGRE,          "Shrek",              15 },
    { MONSTER_OGRE,          "Donkey",              8 },
    { MONSTER_DRAGON,        "Puff",               10 },
    { MONSTER_DRAGON,        "Smaug (Bootleg)",     8 },
    { MONSTER_RANGER_GOBLIN, "Hawkeye (Knockoff)", 10 },
    { MONSTER_WARP_SKULL,    "Skeletor",           12 },
    { MONSTER_DEMON_EYE,     "The Algorithm",      10 },
};
static const int s_nameVariantCount = sizeof(s_nameVariants) / sizeof(s_nameVariants[0]);

static void MaybeAssignMemeName(GameWorld* gw, EntityId e, MonsterType type) {
    CName* n = World_GetName(&gw->ecs, e);
    for (int i = 0; i < s_nameVariantCount; i++) {
        if (s_nameVariants[i].type != type) continue;
        if (GetRandomValue(1, 100) <= s_nameVariants[i].chance) {
            strncpy(n->name, s_nameVariants[i].altName, sizeof(n->name) - 1);
            n->name[sizeof(n->name) - 1] = '\0';
            return;
        }
    }
}

static int DistSq(int ax, int ay, int bx, int by) {
    int dx = ax - bx;
    int dy = ay - by;
    return dx * dx + dy * dy;
}

EntityId SpawnerSystem_SpawnMonster(GameWorld* gw, MonsterType type, int x, int y, int floor) {
    if (!gw) return ENTITY_NONE;
    if ((int)type >= MONSTER_TYPE_COUNT || (int)type < 0) {
        TraceLog(LOG_ERROR, "SpawnerSystem_SpawnMonster: invalid monster type [type=%d]", (int)type);
        return ENTITY_NONE;
    }
    if (!Validate_TilePos(gw, x, y)) {
        TraceLog(LOG_WARNING, "SpawnerSystem_SpawnMonster: tile out of bounds [type=%d x=%d y=%d]", (int)type, x, y);
        return ENTITY_NONE;
    }
    if (floor <= 0) {
        TraceLog(LOG_WARNING, "SpawnerSystem_SpawnMonster: invalid floor [floor=%d]", floor);
        return ENTITY_NONE;
    }
    const MonsterTemplate* tpl = Monster_GetTemplate(type);
    if (!tpl) return ENTITY_NONE;

    EntityId e = World_CreateEntity(&gw->ecs);
    if (e == ENTITY_NONE) {
        TraceLog(LOG_ERROR, "SpawnerSystem_SpawnMonster: World_CreateEntity returned ENTITY_NONE [type=%d at (%d,%d)]", (int)type, x, y);
        return ENTITY_NONE;
    }

    World_AddComponent(&gw->ecs, e, COMP_POSITION);
    CPosition* pos = World_GetPosition(&gw->ecs, e);
    pos->x = x;
    pos->y = y;
    pos->prevX = x;
    pos->prevY = y;
    pos->facingDir = DIR_DOWN;

    int effectiveFloor = floor;
    if (tpl->maxLevel != -1) {
        int maxScaleFloor = tpl->maxLevel - tpl->level;
        if (maxScaleFloor < 1) maxScaleFloor = 1;
        if (effectiveFloor > maxScaleFloor) effectiveFloor = maxScaleFloor;
    }
    float scale = powf(1.12f, (float)effectiveFloor);
    int str = (int)(tpl->str * scale);
    int con = (int)(tpl->con * scale);
    int lck = (int)(tpl->lck * scale);

    World_AddComponent(&gw->ecs, e, COMP_STATS);
    CStats* s = World_GetStats(&gw->ecs, e);
    s->str = str;
    s->dex = (int)(tpl->dex * scale);
    s->intel = (int)(tpl->intel * scale);
    s->con = con;
    s->lck = lck;
    s->maxHp = (int)(tpl->hp * scale) + con * 5;
    s->hp = s->maxHp;
    s->attack = (int)(tpl->attack * scale);
    s->defense = (int)(tpl->defense * scale) + con / 2;
    s->level = tpl->level + floor * GetRandomValue(1, 3);
    if (s->level < 1) s->level = 1;
    if (tpl->maxLevel != -1 && s->level > tpl->maxLevel) s->level = tpl->maxLevel;
    s->expValue = (int)(tpl->expValue * scale) + lck * 3;
    s->alive = true;
    s->statPoints = 0;
    s->exp = 0;
    s->expToNext = 0;

    World_AddComponent(&gw->ecs, e, COMP_SPRITE_ANIM);
    CSpriteAnim* spr = World_GetSprite(&gw->ecs, e);
    {
        const AtlasEntry* ae = Atlas_GetMonster(type);
        spr->tex = ae ? ae->texture : ((tpl->spritePath && tpl->frameCount > 0) ? Resources_LoadTexture(tpl->spritePath) : NULL);
    }
    spr->row = 0;
    spr->frame = 0;
    spr->frameCount = tpl->frameCount;
    spr->animTimer = 0;
    spr->animSpeed = tpl->animSpeed;

    World_AddComponent(&gw->ecs, e, COMP_AI);
    CAI* ai = World_GetAI(&gw->ecs, e);
    ai->type = type;
    ai->attackType = tpl->attackType;
    ai->attackRange = tpl->attackRange;
    ai->detectionRange = tpl->detectionRange;
    ai->lastSeenX = -1;
    ai->lastSeenY = -1;
    ai->huntTurns = 0;
    ai->shadowTurnCounter = 0;

    World_AddComponent(&gw->ecs, e, COMP_NAME);
    CName* n = World_GetName(&gw->ecs, e);
    strncpy(n->name, tpl->name, sizeof(n->name) - 1);
    n->name[sizeof(n->name) - 1] = '\0';
    MaybeAssignMemeName(gw, e, type);

    World_AddComponent(&gw->ecs, e, COMP_FALLBACK_COLOR);
    World_GetColor(&gw->ecs, e)->color = tpl->color;

    World_AddComponent(&gw->ecs, e, COMP_HIT_FLASH);
    World_GetHitFlash(&gw->ecs, e)->timer = 0;

    SpatialHash_Add(gw, e, x, y);
    gw->aliveMonsterCount++;

    DebugLog(DEBUG_SPAWNER, "SpawnMonster: %s type=%d at (%d,%d) floor=%d hp=%d/%d atk=%d def=%d lvl=%d exp=%d",
             n->name, (int)type, x, y, floor, s->hp, s->maxHp, s->attack, s->defense, s->level, s->expValue);

    {
        MonsterSpawnedEvent evt = { e, type, x, y };
        EventBus_Publish(EVT_MONSTER_SPAWNED, &evt);
    }

    return e;
}

EntityId SpawnerSystem_SpawnByTypeName(GameWorld* gw, const char* tmxTypeName, int x, int y, int floor) {
    MonsterType type = Monster_FindTypeByTmxName(tmxTypeName);
    if (type >= MONSTER_TYPE_COUNT) return ENTITY_NONE;
    return SpawnerSystem_SpawnMonster(gw, type, x, y, floor);
}

void SpawnerSystem_ConfigureShadow(GameWorld* gw, EntityId shadow, int playerLevel) {
    if (!gw || shadow == ENTITY_NONE) return;
    if (!World_HasComponents(&gw->ecs, shadow, COMP_STATS | COMP_AI)) return;

    int targetLevel = playerLevel * 2;
    if (targetLevel < 10) targetLevel = 10;

    float scale = powf(1.12f, (float)(targetLevel - 1));
    CStats* s = World_GetStats(&gw->ecs, shadow);
    s->str = (int)(4 * scale);
    s->dex = (int)(6 * scale);
    s->intel = (int)(2 * scale);
    s->con = (int)(3 * scale);
    s->lck = (int)(5 * scale);
    s->level = targetLevel;
    s->maxHp = 10 + s->con * 5;
    s->hp = s->maxHp;
    s->attack = 4 + s->str * 2;
    s->defense = 1 + s->con / 2;
    s->expValue = 10 + s->lck * 3;

    CAI* ai = World_GetAI(&gw->ecs, shadow);
    ai->shadowTurnCounter = 0;
}

void SpawnerSystem_SpawnMonsters(GameWorld* gw, const ProceduralRoom* rooms, int roomCount,
                                 int playerX, int playerY) {
    if (!gw || !gw->map || roomCount == 0) return;

    int* tiles = gw->map->layers[0].data;
    int w = gw->map->width;
    int px = playerX;
    int py = playerY;
    int minDist = MIN_PLAYER_DIST * MIN_PLAYER_DIST;

    #define ROOM_MAX_FLOORS 256

    for (int r = 0; r < roomCount; r++) {
        int floorTiles[ROOM_MAX_FLOORS];
        int floorCount = 0;

        for (int y = rooms[r].y; y < rooms[r].y + rooms[r].h; y++)
            for (int x = rooms[r].x; x < rooms[r].x + rooms[r].w; x++)
                if (IsFloorGID(tiles[y * w + x]) && tiles[y * w + x] != TILE_STAIRS && floorCount < ROOM_MAX_FLOORS)
                    floorTiles[floorCount++] = y * w + x;

        if (floorCount < 4) continue;

        for (int i = floorCount - 1; i > 0; i--) {
            int j = GetRandomValue(0, i);
            int t = floorTiles[i]; floorTiles[i] = floorTiles[j]; floorTiles[j] = t;
        }

        int monsterCount = 1;
        if (floorCount >= 20) monsterCount = 2;
        if (floorCount >= 40) monsterCount = 3;
        if (monsterCount > floorCount) monsterCount = floorCount;

        for (int m = 0; m < monsterCount; m++) {
            int mx = floorTiles[m] % w;
            int my = floorTiles[m] / w;

            if (DistSq(mx, my, px, py) < minDist) continue;

            if (World_FindMonsterAt(gw, mx, my, ENTITY_NONE) != ENTITY_NONE) continue;

            MonsterType type = MONSTER_BAT;
            float totalWeight = 0;
            float monsterWeights[MONSTER_TYPE_COUNT];
            int floor = gw->currentFloor;

            for (int t = 0; t < MONSTER_TYPE_COUNT; t++) {
                const MonsterTemplate* tpl = Monster_GetTemplate((MonsterType)t);
                if (!tpl || tpl->spawnWeight <= 0 || floor < tpl->minFloor ||
                    (tpl->maxFloor != -1 && floor > tpl->maxFloor)) {
                    monsterWeights[t] = 0;
                    continue;
                }
                monsterWeights[t] = (float)tpl->spawnWeight;
                totalWeight += monsterWeights[t];
            }

            if (totalWeight > 0) {
                float roll = (float)GetRandomValue(0, (int)(totalWeight * 100)) / 100.0f;
                float acc = 0;
                for (int t = 0; t < MONSTER_TYPE_COUNT; t++) {
                    if (monsterWeights[t] <= 0) continue;
                    acc += monsterWeights[t];
                    if (roll <= acc) { type = (MonsterType)t; break; }
                }
            }

            EntityId spawned = SpawnerSystem_SpawnMonster(gw, type, mx, my, floor);
            if (spawned != ENTITY_NONE) {
                const MonsterTemplate* dt = Monster_GetTemplate(type);
                DebugLog(DEBUG_SPAWNER, "SpawnFallback: %s type=%d at (%d,%d) floor=%d",
                         dt ? dt->name : "?", (int)type, mx, my, floor);
                gw->monstersEverSpawned = true;
            }
        }
    }

    #undef ROOM_MAX_FLOORS

    int aliveCount = World_CountAliveMonsters(gw);
    DebugLog(DEBUG_SPAWNER, "SpawnFallback: total=%d monsters spawned", aliveCount);
    TraceLog(LOG_INFO, "Spawner: %d monsters placed", aliveCount);
}

#define MAX_MONSTERS 100

void SpawnMonstersForFloor(GameWorld* game) {
    if (!game || !game->map) return;

    const BiomeDef* biome = GetBiomeData(game->currentBiome);
    if (!biome || biome->monsterCount == 0) {
        // Fall back to old spawner
        ProceduralRoom rooms[MAX_GENERATED_ROOMS];
        int roomCount = GetGeneratedRooms(rooms, MAX_GENERATED_ROOMS);
        if (roomCount > 0) {
            int px = 0, py = 0;
            if (game->playerEntity != ENTITY_NONE) {
                CPosition* pp = World_GetPosition(&game->ecs, game->playerEntity);
                px = pp->x; py = pp->y;
            }
            SpawnerSystem_SpawnMonsters(game, rooms, roomCount, px, py);
        }
        return;
    }

    int floorNumber = game->currentFloor;
    float budget = Monster_GetFloorBudget(floorNumber);
    float remaining = budget;
    int spawned = 0;

    int* tiles = game->map->layers[0].data;
    int mapW = game->map->width;
    int mapH = game->map->height;

    int playerX = 0, playerY = 0;
    if (game->playerEntity != ENTITY_NONE) {
        CPosition* pp = World_GetPosition(&game->ecs, game->playerEntity);
        playerX = pp->x; playerY = pp->y;
    }
    int minDist = MIN_PLAYER_DIST * MIN_PLAYER_DIST;

    while (remaining >= 0.125f && spawned < MAX_MONSTERS) {
        // Build candidate list: monsters in biome pool whose scaled CR <= remaining
        int candidates[MONSTER_TYPE_COUNT];
        float candidateCRs[MONSTER_TYPE_COUNT];
        int candidateCount = 0;

        for (int i = 0; i < biome->monsterCount; i++) {
            MonsterType mt = biome->monsterPool[i];
            const MonsterTemplate* def = Monster_GetTemplate(mt);
            if (!def || def->spawnWeight <= 0) continue;
            if (floorNumber < def->minFloor) continue;
            if (def->maxFloor != -1 && floorNumber > def->maxFloor) continue;
            float cr = Monster_CalcCR(def, floorNumber);
            if (cr <= remaining + 0.001f) {
                candidates[candidateCount] = (int)mt;
                candidateCRs[candidateCount] = cr;
                candidateCount++;
            }
        }

        if (candidateCount == 0) break;

        // Weight inversely by CR (cheaper monsters more likely to fill budget)
        float invWeights[MONSTER_TYPE_COUNT];
        float totalInvWeight = 0.0f;
        for (int i = 0; i < candidateCount; i++) {
            invWeights[i] = (candidateCRs[i] > 0.0f) ? 1.0f / candidateCRs[i] : 10.0f;
            totalInvWeight += invWeights[i];
        }

        float roll = (float)GetRandomValue(0, (int)(totalInvWeight * 100)) / 100.0f;
        float acc = 0.0f;
        int chosenIdx = 0;
        for (int i = 0; i < candidateCount; i++) {
            acc += invWeights[i];
            if (roll <= acc) { chosenIdx = i; break; }
        }

        MonsterType chosenType = (MonsterType)candidates[chosenIdx];
        const MonsterTemplate* def = Monster_GetTemplate(chosenType);
        if (!def) break;

        // Find valid spawn tile (not occupied, walkable, not near player)
        int sx = -1, sy = -1;
        for (int attempt = 0; attempt < 200; attempt++) {
            int tx = GetRandomValue(0, mapW - 1);
            int ty = GetRandomValue(0, mapH - 1);
            if (tx < 0 || tx >= mapW || ty < 0 || ty >= mapH) continue;
            if (game->blocking[ty][tx]) continue;
            if (!IsFloorGID(tiles[ty * mapW + tx]) || tiles[ty * mapW + tx] == TILE_STAIRS) continue;
            if (World_FindMonsterAt(game, tx, ty, ENTITY_NONE) != ENTITY_NONE) continue;
            if (playerX >= 0 && DistSq(tx, ty, playerX, playerY) < minDist) continue;
            sx = tx; sy = ty; break;
        }
        if (sx < 0) break;

        // Scale stats for floor
        float scale = 1.0f + (float)(floorNumber - def->minFloor) * 0.10f;
        if (scale < 1.0f) scale = 1.0f;
        int scaledHp      = (int)((float)def->hp      * scale);
        int scaledAttack  = (int)((float)def->attack  * scale);
        int scaledDefense = (int)((float)def->defense * scale);

        // Assign equipment
        EquipType chosenWeapon = EQUIP_NONE;
        EquipType chosenArmor  = EQUIP_NONE;
        float monsterShare = remaining / (budget * 0.3f);
        if (monsterShare > 1.0f) monsterShare = 1.0f;
        if (monsterShare < 0.0f) monsterShare = 0.0f;

        if (def->weaponPoolCount > 0) {
            int wi = (int)(monsterShare * (float)(def->weaponPoolCount - 1) + 0.5f);
            if (wi >= def->weaponPoolCount) wi = def->weaponPoolCount - 1;
            chosenWeapon = def->weaponPool[wi];
        }
        if (floorNumber == 1 && chosenType == MONSTER_GOBLIN && chosenWeapon == EQUIP_SIMPLE_BOW)
            chosenWeapon = EQUIP_DAGGER;
        if (def->armorPoolCount > 0) {
            int ai = (int)(monsterShare * (float)(def->armorPoolCount - 1) + 0.5f);
            if (ai >= def->armorPoolCount) ai = def->armorPoolCount - 1;
            chosenArmor = def->armorPool[ai];
        }

        // Spawn entity
        EntityId e = World_CreateEntity(&game->ecs);
        if (e == ENTITY_NONE) break;

        World_AddComponent(&game->ecs, e, COMP_POSITION);
        CPosition* pos = World_GetPosition(&game->ecs, e);
        pos->x = sx; pos->y = sy;
        pos->prevX = sx; pos->prevY = sy;
        pos->facingDir = DIR_DOWN;

        World_AddComponent(&game->ecs, e, COMP_STATS);
        CStats* s = World_GetStats(&game->ecs, e);
        s->str = def->str;
        s->dex = def->dex;
        s->intel = def->intel;
        s->con = def->con;
        s->lck = def->lck;
        s->maxHp = scaledHp + def->con * 5;
        s->hp = s->maxHp;
        s->attack = scaledAttack;
        s->defense = scaledDefense + def->con / 2;
        s->level = def->level + floorNumber * GetRandomValue(1, 3);
        if (s->level < 1) s->level = 1;
        if (def->maxLevel != -1 && s->level > def->maxLevel) s->level = def->maxLevel;
        s->expValue = (int)((float)def->expValue * scale) + def->lck * 3;
        s->alive = true;
        s->statPoints = 0;
        s->exp = 0;
        s->expToNext = 0;

        World_AddComponent(&game->ecs, e, COMP_SPRITE_ANIM);
        CSpriteAnim* spr = World_GetSprite(&game->ecs, e);
        {
            const AtlasEntry* ae = Atlas_GetMonster(chosenType);
            spr->tex = ae ? ae->texture : ((def->spritePath && def->frameCount > 0) ? Resources_LoadTexture(def->spritePath) : NULL);
        }
        spr->row = 0;
        spr->frame = 0;
        spr->frameCount = def->frameCount;
        spr->animTimer = 0;
        spr->animSpeed = def->animSpeed;

        World_AddComponent(&game->ecs, e, COMP_AI);
        CAI* ai = World_GetAI(&game->ecs, e);
        ai->type = chosenType;
        ai->attackType = def->attackType;
        ai->attackRange = def->attackRange;
        ai->detectionRange = def->detectionRange;
        ai->lastSeenX = -1;
        ai->lastSeenY = -1;
        ai->huntTurns = 0;
        ai->shadowTurnCounter = 0;
        ai->equippedWeapon = chosenWeapon;
        ai->equippedArmor  = chosenArmor;

        World_AddComponent(&game->ecs, e, COMP_NAME);
        CName* n = World_GetName(&game->ecs, e);
        strncpy(n->name, def->name, sizeof(n->name) - 1);
        n->name[sizeof(n->name) - 1] = '\0';
        MaybeAssignMemeName(game, e, chosenType);

        World_AddComponent(&game->ecs, e, COMP_FALLBACK_COLOR);
        World_GetColor(&game->ecs, e)->color = def->color;

        World_AddComponent(&game->ecs, e, COMP_HIT_FLASH);
        World_GetHitFlash(&game->ecs, e)->timer = 0;

        // Apply equipment bonuses
        if (chosenWeapon != EQUIP_NONE)
            EquipmentBonus_Apply(&game->ecs, e, chosenWeapon);
        if (chosenArmor != EQUIP_NONE)
            EquipmentBonus_Apply(&game->ecs, e, chosenArmor);

        // Assign CAbilities from weapon
        World_AddComponent(&game->ecs, e, COMP_ABILITIES);
        CAbilities* a = World_GetAbilities(&game->ecs, e);
        a->abilities[0] = Inventory_GetWeaponAbility(chosenWeapon);
        a->count = 1;
        a->maxMp = MP_BASE + def->intel * MP_PER_INT;
        a->mp = a->maxMp;

        SpatialHash_Add(game, e, sx, sy);
        game->aliveMonsterCount++;
        game->monstersEverSpawned = true;

        float cr = Monster_CalcCR(def, floorNumber);
        DebugLog(DEBUG_SPAWNER, "SpawnBudget: %s type=%d at (%d,%d) floor=%d CR=%.2f hp=%d/%d atk=%d def=%d lvl=%d weapon=%d armor=%d",
                 n->name, (int)chosenType, sx, sy, floorNumber, cr,
                 s->hp, s->maxHp, s->attack, s->defense, s->level,
                 (int)chosenWeapon, (int)chosenArmor);

        {
            MonsterSpawnedEvent evt = { e, chosenType, sx, sy };
            EventBus_Publish(EVT_MONSTER_SPAWNED, &evt);
        }

        remaining -= cr;
        spawned++;
    }

    DebugLog(DEBUG_SPAWNER, "SpawnBudget: total CR=%.2f spent=%.2f count=%d",
             budget, budget - remaining, spawned);
    if (spawned == 0) TraceLog(LOG_WARNING, "SpawnMonstersForFloor: 0 monsters spawned despite budget=%.2f [floor=%d]", budget, floorNumber);
    TraceLog(LOG_INFO, "Spawner(budget): %d monsters placed (%.1f budget remaining)", spawned, remaining);
}

void SpawnShopRoom(GameWorld* game)
{
    if (!game || !game->map) return;
    if (game->currentFloor < 2) return;

    ProceduralRoom rooms[MAX_GENERATED_ROOMS];
    int roomCount = GetGeneratedRooms(rooms, MAX_GENERATED_ROOMS);
    if (roomCount == 0) return;

    int* tiles = game->map->layers[0].data;
    int mapW = game->map->width;

    int playerX = 0, playerY = 0;
    if (game->playerEntity != ENTITY_NONE) {
        CPosition* pp = World_GetPosition(&game->ecs, game->playerEntity);
        playerX = pp->x; playerY = pp->y;
    }

    for (int r = 0; r < roomCount; r++) {
        // Skip player's room
        if (playerX >= rooms[r].x && playerX < rooms[r].x + rooms[r].w &&
            playerY >= rooms[r].y && playerY < rooms[r].y + rooms[r].h)
            continue;

        // Check if room has monsters
        bool hasMonster = false;
        for (int y = rooms[r].y; y < rooms[r].y + rooms[r].h && !hasMonster; y++)
            for (int x = rooms[r].x; x < rooms[r].x + rooms[r].w && !hasMonster; x++)
                if (World_FindMonsterAt(game, x, y, ENTITY_NONE) != ENTITY_NONE)
                    hasMonster = true;

        if (hasMonster) continue;

        // Empty room — place shopkeeper at center
        int cx = rooms[r].x + rooms[r].w / 2;
        int cy = rooms[r].y + rooms[r].h / 2;
        if (cx < 0 || cx >= mapW || cy < 0 || cy >= game->map->height) continue;
        if (!IsFloorGID(tiles[cy * mapW + cx])) continue;

        EntityId e = World_CreateEntity(&game->ecs);
        if (e == ENTITY_NONE) return;

        World_AddComponent(&game->ecs, e, COMP_POSITION);
        CPosition* pos = World_GetPosition(&game->ecs, e);
        pos->x = cx; pos->y = cy;
        pos->prevX = cx; pos->prevY = cy;

        World_AddComponent(&game->ecs, e, COMP_FALLBACK_COLOR);
        World_GetColor(&game->ecs, e)->color = (Color){ 255, 215, 0, 255 };

        World_AddComponent(&game->ecs, e, COMP_NAME);
        CName* n = World_GetName(&game->ecs, e);
        strncpy(n->name, "Shopkeeper", sizeof(n->name) - 1);

        game->shopkeeperEntity = e;
        DebugLog(DEBUG_SPAWNER, "ShopSpawn: at (%d,%d) room=%d floor=%d", cx, cy, r, game->currentFloor);
        TraceLog(LOG_INFO, "Shop room spawned at (%d, %d)", cx, cy);
        return;
    }
}

void SpawnerSystem_SpawnPickups(GameWorld* gw) {
    if (!gw || !gw->map) return;

    ProceduralRoom rooms[MAX_GENERATED_ROOMS];
    int roomCount = GetGeneratedRooms(rooms, MAX_GENERATED_ROOMS);
    if (roomCount == 0) return;

    int playerX = 0, playerY = 0;
    int playerLck = 0;
    if (gw->playerEntity != ENTITY_NONE) {
        CPosition* pPos = World_GetPosition(&gw->ecs, gw->playerEntity);
        if (pPos) { playerX = pPos->x; playerY = pPos->y; }
        CStats* ps = World_GetStats(&gw->ecs, gw->playerEntity);
        if (ps) playerLck = ps->lck;
    }

    int* tiles = gw->map->layers[0].data;
    int w = gw->map->width;
    int minDist = MIN_PLAYER_DIST * MIN_PLAYER_DIST;

    #define ROOM_MAX_FLOORS 256

    for (int r = 0; r < roomCount; r++) {
        int floorTiles[ROOM_MAX_FLOORS];
        int floorCount = 0;

        for (int y = rooms[r].y; y < rooms[r].y + rooms[r].h; y++)
            for (int x = rooms[r].x; x < rooms[r].x + rooms[r].w; x++)
                if (IsFloorGID(tiles[y * w + x]) && tiles[y * w + x] != TILE_STAIRS && floorCount < ROOM_MAX_FLOORS)
                    floorTiles[floorCount++] = y * w + x;

        if (floorCount < 4) continue;

        for (int i = floorCount - 1; i > 0; i--) {
            int j = GetRandomValue(0, i);
            int t = floorTiles[i]; floorTiles[i] = floorTiles[j]; floorTiles[j] = t;
        }

        int lootSlots = (floorCount >= 60) ? 2 : 1;
        for (int li = 0; li < lootSlots; li++) {
            int fi = floorCount - 1 - li;
            if (fi < 0) break;
            int lx = floorTiles[fi] % w;
            int ly = floorTiles[fi] / w;
            if (DistSq(lx, ly, playerX, playerY) < minDist) continue;

            int tierMinFloor[4] = { 1, 3, 5, 7 };
            float tierWeights[4] = { 0, 0, 0, 0 };
            for (int te = 0; te < (int)LOOT_TABLE_COUNT; te++) {
                const LootEntry* entry = &LOOT_TABLE[te];
                int ti = entry->tier - 1;
                if (gw->currentFloor < tierMinFloor[ti]) continue;
                float luckBonus = (float)playerLck * 2.0f * (float)(ti + 1);
                float floorBonus = (float)gw->currentFloor * 3.0f * (float)(ti + 1);
                float effectiveWeight = (float)entry->baseWeight + luckBonus + floorBonus;
                if (effectiveWeight < 0) effectiveWeight = 0;
                tierWeights[ti] += effectiveWeight;
            }

            float tierTotal = tierWeights[0] + tierWeights[1] + tierWeights[2] + tierWeights[3];
            if (tierTotal <= 0) continue;

            float tierRoll = (float)GetRandomValue(0, (int)(tierTotal * 100)) / 100.0f;
            int chosenTier = 0;
            float accTier = 0;
            for (int ti = 0; ti < 4; ti++) {
                accTier += tierWeights[ti];
                if (tierRoll <= accTier) { chosenTier = ti + 1; break; }
            }
            if (chosenTier == 0) chosenTier = 1;

            int tierEntryCount = 0;
            int tierEntryIndices[32];
            float tierEntryBaseWeights[32];
            float tierEntryTotal = 0;
            for (int te = 0; te < (int)LOOT_TABLE_COUNT; te++) {
                if (LOOT_TABLE[te].tier == chosenTier && tierEntryCount < 32) {
                    tierEntryIndices[tierEntryCount] = te;
                    float fw = (float)LOOT_TABLE[te].baseWeight;
                    tierEntryBaseWeights[tierEntryCount] = fw;
                    tierEntryTotal += fw;
                    tierEntryCount++;
                }
            }
            if (tierEntryCount == 0) continue;

            float itemRoll = (float)GetRandomValue(0, (int)(tierEntryTotal * 100)) / 100.0f;
            float accItem = 0;
            int chosenEntryIdx = 0;
            for (int te = 0; te < tierEntryCount; te++) {
                accItem += tierEntryBaseWeights[te];
                if (itemRoll <= accItem) { chosenEntryIdx = tierEntryIndices[te]; break; }
            }

            const LootEntry* chosen = &LOOT_TABLE[chosenEntryIdx];

            EntityId e = World_CreateEntity(&gw->ecs);
            if (e == ENTITY_NONE) continue;

            World_AddComponent(&gw->ecs, e, COMP_POSITION);
            CPosition* pos = World_GetPosition(&gw->ecs, e);
            pos->x = lx; pos->y = ly;
            pos->prevX = lx; pos->prevY = ly;

            World_AddComponent(&gw->ecs, e, COMP_PICKUP);
            CPickup* pk = World_GetPickup(&gw->ecs, e);
            pk->isEquip = (chosen->cat == LOOT_TYPE_EQUIP);
            if (pk->isEquip) pk->equipType = (EquipType)chosen->typeId;
            else pk->itemType = (ItemType)chosen->typeId;
            pk->quantity = 1;
            DebugLog(DEBUG_SPAWNER, "SpawnPickup: %sqty=%d at (%d,%d) tier=%d",
                     pk->isEquip ? "equip=" : "potion=",
                     pk->quantity, lx, ly, chosenTier);
        }
    }

    #undef ROOM_MAX_FLOORS
}

EntityId SpawnerSystem_FindPickupAt(GameWorld* gw, int x, int y) {
    if (!gw) return ENTITY_NONE;
    if (!Validate_TilePos(gw, x, y)) { TraceLog(LOG_WARNING, "SpawnerSystem_FindPickupAt: tile out of bounds [x=%d y=%d]", x, y); return ENTITY_NONE; }
    for (EntityId e = 1; e < (EntityId)gw->ecs.count; e++) {
        if (!gw->ecs.alive[e]) continue;
        if (!World_HasComponents(&gw->ecs, e, COMP_POSITION | COMP_PICKUP)) continue;
        CPosition* p = World_GetPosition(&gw->ecs, e);
        CPickup* pk = World_GetPickup(&gw->ecs, e);
        if (pk->quantity > 0 && p->x == x && p->y == y) return e;
    }
    return ENTITY_NONE;
}

int SpawnerSystem_CollectPickupsAt(GameWorld* gw, int x, int y, ItemType* outTypes, int* outQtys, int maxSlots) {
    int found = 0;
    for (EntityId e = 1; e < (EntityId)gw->ecs.count; e++) {
        if (!gw->ecs.alive[e]) continue;
        if (!World_HasComponents(&gw->ecs, e, COMP_POSITION | COMP_PICKUP)) continue;
        CPosition* p = World_GetPosition(&gw->ecs, e);
        CPickup* pk = World_GetPickup(&gw->ecs, e);
        if (pk->quantity > 0 && !pk->isEquip && p->x == x && p->y == y && found < maxSlots) {
            outTypes[found] = pk->itemType;
            outQtys[found] = pk->quantity;
            found++;
            // FIX: Destroy entity after collection to prevent ghost entities
            World_DestroyEntity(&gw->ecs, e);
        }
    }
    return found;
}

int SpawnerSystem_CollectEquipAt(GameWorld* gw, int x, int y, EquipType* outTypes, int* outQtys, int maxSlots) {
    int found = 0;
    for (EntityId e = 1; e < (EntityId)gw->ecs.count; e++) {
        if (!gw->ecs.alive[e]) continue;
        if (!World_HasComponents(&gw->ecs, e, COMP_POSITION | COMP_PICKUP)) continue;
        CPosition* p = World_GetPosition(&gw->ecs, e);
        CPickup* pk = World_GetPickup(&gw->ecs, e);
        if (pk->quantity > 0 && pk->isEquip && p->x == x && p->y == y && found < maxSlots) {
            outTypes[found] = pk->equipType;
            outQtys[found] = pk->quantity;
            World_DestroyEntity(&gw->ecs, e);
            found++;
        }
    }
    return found;
}

void SpawnerSystem_ReduceEquipAt(GameWorld* gw, int x, int y, EquipType type, int amount) {
    if (amount <= 0) return;
    for (EntityId e = 1; e < (EntityId)gw->ecs.count; e++) {
        if (!gw->ecs.alive[e]) continue;
        if (!World_HasComponents(&gw->ecs, e, COMP_POSITION | COMP_PICKUP)) continue;
        CPosition* p = World_GetPosition(&gw->ecs, e);
        CPickup* pk = World_GetPickup(&gw->ecs, e);
        if (pk->quantity > 0 && pk->isEquip && pk->equipType == type &&
            p->x == x && p->y == y) {
            pk->quantity -= amount;
            if (pk->quantity < 0) pk->quantity = 0;
            return;
        }
    }
}

static EntityId FindStackablePickup(GameWorld* gw, int x, int y, bool isEquip, int typeId) {
    for (EntityId e = 1; e < (EntityId)gw->ecs.count; e++) {
        if (!gw->ecs.alive[e]) continue;
        if (!World_HasComponents(&gw->ecs, e, COMP_POSITION | COMP_PICKUP)) continue;
        CPosition* p = World_GetPosition(&gw->ecs, e);
        CPickup* pk = World_GetPickup(&gw->ecs, e);
        if (pk->quantity <= 0 || p->x != x || p->y != y || pk->isEquip != isEquip) continue;
        if (isEquip) {
            if ((int)pk->equipType == typeId) return e;
        } else {
            if ((int)pk->itemType == typeId) return e;
        }
    }
    return ENTITY_NONE;
}

static EntityId CreatePickupEntity(GameWorld* gw, int x, int y, bool isEquip, int typeId, int qty) {
    EntityId e = World_CreateEntity(&gw->ecs);
    if (e == ENTITY_NONE) { TraceLog(LOG_WARNING, "CreatePickupEntity: no free entity slot [isEquip=%d type=%d at (%d,%d)]", (int)isEquip, typeId, x, y); return ENTITY_NONE; }

    World_AddComponent(&gw->ecs, e, COMP_POSITION);
    CPosition* pos = World_GetPosition(&gw->ecs, e);
    pos->x = x;
    pos->y = y;
    pos->prevX = x;
    pos->prevY = y;

    World_AddComponent(&gw->ecs, e, COMP_PICKUP);
    CPickup* pk = World_GetPickup(&gw->ecs, e);
    pk->isEquip = isEquip;
    if (isEquip) pk->equipType = (EquipType)typeId;
    else pk->itemType = (ItemType)typeId;
    pk->quantity = qty;

    return e;
}

void SpawnerSystem_AddPotionAt(GameWorld* gw, int x, int y, ItemType type, int qty) {
    if (!gw || qty <= 0) return;
    EntityId existing = FindStackablePickup(gw, x, y, false, (int)type);
    if (existing != ENTITY_NONE) {
        World_GetPickup(&gw->ecs, existing)->quantity += qty;
        return;
    }
    CreatePickupEntity(gw, x, y, false, (int)type, qty);
}

void SpawnerSystem_AddEquipAt(GameWorld* gw, int x, int y, EquipType type, int qty) {
    if (!gw || qty <= 0) return;
    EntityId existing = FindStackablePickup(gw, x, y, true, (int)type);
    if (existing != ENTITY_NONE) {
        World_GetPickup(&gw->ecs, existing)->quantity += qty;
        return;
    }
    CreatePickupEntity(gw, x, y, true, (int)type, qty);
}

void SpawnerSystem_SpawnHealingPotionAt(GameWorld* gw, int x, int y) {
    int roll = GetRandomValue(1, 100);
    ItemType type;
    if (roll <= 50) type = ITEM_SMALL_HP_POTION;
    else if (roll <= 80) type = ITEM_MEDIUM_HP_POTION;
    else type = ITEM_LARGE_HP_POTION;
    SpawnerSystem_AddPotionAt(gw, x, y, type, 1);
}

int SpawnerSystem_ListPotionsAt(GameWorld* gw, int x, int y,
                                ItemType* outTypes, int* outQtys, int maxSlots) {
    int found = 0;
    for (EntityId e = 1; e < (EntityId)gw->ecs.count; e++) {
        if (!gw->ecs.alive[e]) continue;
        if (!World_HasComponents(&gw->ecs, e, COMP_POSITION | COMP_PICKUP)) continue;
        CPosition* p = World_GetPosition(&gw->ecs, e);
        CPickup* pk = World_GetPickup(&gw->ecs, e);
        if (pk->quantity > 0 && !pk->isEquip && p->x == x && p->y == y && found < maxSlots) {
            outTypes[found] = pk->itemType;
            outQtys[found] = pk->quantity;
            found++;
        }
    }
    return found;
}

int SpawnerSystem_ListEquipAt(GameWorld* gw, int x, int y,
                              EquipType* outTypes, int* outQtys, int maxSlots) {
    int found = 0;
    for (EntityId e = 1; e < (EntityId)gw->ecs.count; e++) {
        if (!gw->ecs.alive[e]) continue;
        if (!World_HasComponents(&gw->ecs, e, COMP_POSITION | COMP_PICKUP)) continue;
        CPosition* p = World_GetPosition(&gw->ecs, e);
        CPickup* pk = World_GetPickup(&gw->ecs, e);
        if (pk->quantity > 0 && pk->isEquip && p->x == x && p->y == y && found < maxSlots) {
            outTypes[found] = pk->equipType;
            outQtys[found] = pk->quantity;
            found++;
        }
    }
    return found;
}
