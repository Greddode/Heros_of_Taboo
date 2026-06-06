#include "ecs.h"
#include <string.h>

static void ZeroEntityComponents(World* w, EntityId e) {
    memset(&w->positions[e], 0, sizeof(CPosition));
    memset(&w->stats[e], 0, sizeof(CStats));
    memset(&w->sprites[e], 0, sizeof(CSpriteAnim));
    memset(&w->colors[e], 0, sizeof(CFallbackColor));
    memset(&w->ais[e], 0, sizeof(CAI));
    memset(&w->pickups[e], 0, sizeof(CPickup));
    memset(&w->names[e], 0, sizeof(CName));
    memset(&w->hitFlashes[e], 0, sizeof(CHitFlash));
    memset(&w->abilities[e], 0, sizeof(CAbilities));
}

void World_Init(World* w) {
    w->count = 1; // slot 0 is ENTITY_NONE (reserved)
    w->freeCount = 0;
    memset(w->masks, 0, sizeof(w->masks));
    memset(w->alive, 0, sizeof(w->alive));
}

EntityId World_CreateEntity(World* w) {
    EntityId e;
    if (w->freeCount > 0) {
        e = w->freeList[--w->freeCount];
    } else {
        if (w->count >= MAX_ENTITIES) {
            TraceLog(LOG_ERROR, "World_CreateEntity: entity pool exhausted (MAX_ENTITIES=%d) — critical failure", MAX_ENTITIES);
            return ENTITY_NONE;
        }
        e = w->count++;
    }
    w->masks[e] = 0;
    w->alive[e] = true;
    ZeroEntityComponents(w, e);
    return e;
}

void World_DestroyEntity(World* w, EntityId e) {
    if (e == ENTITY_NONE || e >= (EntityId)w->count) return;
    ZeroEntityComponents(w, e);
    w->masks[e] = 0;
    w->alive[e] = false;
    w->freeList[w->freeCount++] = e;
}

bool World_IsAlive(const World* w, EntityId e) {
    if (e == ENTITY_NONE || e >= (EntityId)w->count) return false;
    return w->alive[e];
}

bool World_HasComponents(const World* w, EntityId e, ComponentMask mask) {
    if (e == ENTITY_NONE || e >= (EntityId)w->count) return false;
    return w->alive[e] && (w->masks[e] & mask) == mask;
}
