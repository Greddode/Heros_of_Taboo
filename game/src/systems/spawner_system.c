#include "spawner_system.h"
#include "core/game.h"
#include "entity/monster.h"
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

void SpawnerSystem_SpawnMonsters(GameWorld* gw, const ProceduralRoom* rooms, int roomCount) {
    if (!gw || !gw->map || roomCount == 0) return;

    int* tiles = gw->map->layers[0].data;
    int w = gw->map->width;
    int h = gw->map->height;

    CPosition* playerPos = World_GetPosition(&gw->ecs, gw->playerEntity);
    int px = playerPos->x;
    int py = playerPos->y;
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

            // Check if any monster already at this tile
            bool occupied = false;
            for (EntityId check = 1; check < (EntityId)gw->ecs.count; check++) {
                if (!gw->ecs.alive[check]) continue;
                if (!World_HasComponents(&gw->ecs, check, COMP_POSITION | COMP_STATS)) continue;
                if (World_HasComponents(&gw->ecs, check, COMP_PLAYER_TAG)) continue;
                if (!World_HasComponents(&gw->ecs, check, COMP_AI)) continue;
                if (World_GetAI(&gw->ecs, check)->type == 0) continue;
                CPosition* cp = World_GetPosition(&gw->ecs, check);
                if (cp->x == mx && cp->y == my) { occupied = true; break; }
            }
            if (occupied) continue;

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

            const MonsterTemplate* tpl = Monster_GetTemplate(type);
            if (!tpl) continue;

            EntityId e = World_CreateEntity(&gw->ecs);
            if (e == ENTITY_NONE) continue;

            World_AddComponent(&gw->ecs, e, COMP_POSITION);
            CPosition* pos = World_GetPosition(&gw->ecs, e);
            pos->x = mx; pos->y = my;
            pos->prevX = mx; pos->prevY = my;
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
            s->exp = 0; s->expToNext = 0;

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
            CFallbackColor* col = World_GetColor(&gw->ecs, e);
            col->color = tpl->color;

            World_AddComponent(&gw->ecs, e, COMP_HIT_FLASH);
            CHitFlash* hf = World_GetHitFlash(&gw->ecs, e);
            hf->timer = 0;
        }
    }

    #undef ROOM_MAX_FLOORS
}

void SpawnerSystem_SpawnPickups(GameWorld* gw, Game* game) {
    if (!gw || !game) return;

    // Spawn potion entities from old arrays
    for (int i = 0; i < game->potionCount; i++) {
        if (game->potionCollected[i]) continue;
        if (game->potionQuantities[i] <= 0) continue;

        int px = game->potionTiles[i][0];
        int py = game->potionTiles[i][1];

        EntityId e = World_CreateEntity(&gw->ecs);
        if (e == ENTITY_NONE) continue;

        World_AddComponent(&gw->ecs, e, COMP_POSITION);
        CPosition* pos = World_GetPosition(&gw->ecs, e);
        pos->x = px; pos->y = py;
        pos->prevX = px; pos->prevY = py;

        World_AddComponent(&gw->ecs, e, COMP_PICKUP);
        CPickup* pk = World_GetPickup(&gw->ecs, e);
        pk->isEquip = false;
        pk->itemType = game->potionTypes[i];
        pk->quantity = game->potionQuantities[i];
    }

    // Spawn equipment entities from old arrays
    for (int i = 0; i < game->equipMapCount; i++) {
        if (game->equipMapCollected[i]) continue;
        if (game->equipMapQuantities[i] <= 0) continue;

        int px = game->equipMapTiles[i][0];
        int py = game->equipMapTiles[i][1];

        EntityId e = World_CreateEntity(&gw->ecs);
        if (e == ENTITY_NONE) continue;

        World_AddComponent(&gw->ecs, e, COMP_POSITION);
        CPosition* pos = World_GetPosition(&gw->ecs, e);
        pos->x = px; pos->y = py;
        pos->prevX = px; pos->prevY = py;

        World_AddComponent(&gw->ecs, e, COMP_PICKUP);
        CPickup* pk = World_GetPickup(&gw->ecs, e);
        pk->isEquip = true;
        pk->equipType = game->equipMapTypes[i];
        pk->quantity = game->equipMapQuantities[i];
    }
}

EntityId SpawnerSystem_FindPickupAt(const GameWorld* gw, int x, int y) {
    for (EntityId e = 1; e < (EntityId)gw->ecs.count; e++) {
        if (!gw->ecs.alive[e]) continue;
        if (!World_HasComponents(&((GameWorld*)gw)->ecs, e, COMP_POSITION | COMP_PICKUP)) continue;
        CPosition* p = World_GetPosition(&((GameWorld*)gw)->ecs, e);
        CPickup* pk = World_GetPickup(&((GameWorld*)gw)->ecs, e);
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
            pk->quantity = 0;
            found++;
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
            pk->quantity = 0;
            found++;
        }
    }
    return found;
}
