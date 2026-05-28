#include "ecs.h"
#include <string.h>

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
            return ENTITY_NONE;
        }
        e = w->count++;
    }
    w->masks[e] = 0;
    w->alive[e] = true;
    return e;
}

void World_DestroyEntity(World* w, EntityId e) {
    if (e == ENTITY_NONE || e >= (EntityId)w->count) return;
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
