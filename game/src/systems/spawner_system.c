#include "spawner_system.h"
#include "data/loot_data.h"
#include "data/monster_data.h"
#include "world_monster.h"
#include "resources.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MIN_PLAYER_DIST 5

static int DistSq(int ax, int ay, int bx, int by) {
    int dx = ax - bx;
    int dy = ay - by;
    return dx * dx + dy * dy;
}

EntityId SpawnerSystem_SpawnMonster(GameWorld* gw, MonsterType type, int x, int y, int floor) {
    const MonsterTemplate* tpl = Monster_GetTemplate(type);
    if (!tpl || !gw) return ENTITY_NONE;

    EntityId e = World_CreateEntity(&gw->ecs);
    if (e == ENTITY_NONE) return ENTITY_NONE;

    World_AddComponent(&gw->ecs, e, COMP_POSITION);
    CPosition* pos = World_GetPosition(&gw->ecs, e);
    pos->x = x;
    pos->y = y;
    pos->prevX = x;
    pos->prevY = y;
    pos->facingDir = DIR_DOWN;

    float scale = powf(1.12f, (float)floor);
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
    s->expValue = (int)(tpl->expValue * scale) + lck * 3;
    s->alive = true;
    s->statPoints = 0;
    s->exp = 0;
    s->expToNext = 0;

    World_AddComponent(&gw->ecs, e, COMP_SPRITE_ANIM);
    CSpriteAnim* spr = World_GetSprite(&gw->ecs, e);
    spr->tex = (tpl->spritePath && tpl->frameCount > 0) ? Resources_LoadTexture(tpl->spritePath) : NULL;
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

    World_AddComponent(&gw->ecs, e, COMP_FALLBACK_COLOR);
    World_GetColor(&gw->ecs, e)->color = tpl->color;

    World_AddComponent(&gw->ecs, e, COMP_HIT_FLASH);
    World_GetHitFlash(&gw->ecs, e)->timer = 0;

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
                if (!tpl || tpl->spawnWeight <= 0 || floor < tpl->minFloor) {
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

            SpawnerSystem_SpawnMonster(gw, type, mx, my, floor);
        }
    }

    #undef ROOM_MAX_FLOORS

    TraceLog(LOG_INFO, "Spawner: %d monsters placed", World_CountAliveMonsters(gw));
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

            int tierMinFloor[4] = { 1, 1, 4, 6 };
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

            // FIX: Add Sprite component so RenderSystem can see the pickup
            World_AddComponent(&gw->ecs, e, COMP_SPRITE_ANIM);
            CSpriteAnim* spr = World_GetSprite(&gw->ecs, e);
            // Assuming you have a central sheet for items or specific paths
            spr->tex = Resources_LoadTexture("resources/tilesets/items.png"); 
            spr->frameCount = 0; // Static
            spr->row = pk->isEquip ? 1 : 0;
            spr->frame = chosen->typeId;
        }
    }

    #undef ROOM_MAX_FLOORS
}

EntityId SpawnerSystem_FindPickupAt(GameWorld* gw, int x, int y) {
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
    if (e == ENTITY_NONE) return ENTITY_NONE;

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

    // Add visuals for manual additions
    World_AddComponent(&gw->ecs, e, COMP_SPRITE_ANIM);
    CSpriteAnim* spr = World_GetSprite(&gw->ecs, e);
    spr->tex = Resources_LoadTexture("resources/tilesets/items.png");
    spr->frameCount = 0;
    spr->row = isEquip ? 1 : 0;
    spr->frame = typeId;

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
