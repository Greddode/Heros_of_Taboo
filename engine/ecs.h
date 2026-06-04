#ifndef ECS_H
#define ECS_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "components.h"

#define MAX_ENTITIES 128

typedef uint32_t EntityId;
typedef uint32_t ComponentMask;

#define ENTITY_NONE 0

typedef struct World {
    ComponentMask masks[MAX_ENTITIES];
    bool alive[MAX_ENTITIES];
    int count;
    int freeList[MAX_ENTITIES];
    int freeCount;

    // Component pools (one per component type)
    CPosition      positions[MAX_ENTITIES];
    CStats         stats[MAX_ENTITIES];
    CSpriteAnim    sprites[MAX_ENTITIES];
    CFallbackColor colors[MAX_ENTITIES];
    CAI            ais[MAX_ENTITIES];
    CPickup        pickups[MAX_ENTITIES];
    CName          names[MAX_ENTITIES];
    CHitFlash      hitFlashes[MAX_ENTITIES];
    CAbilities     abilities[MAX_ENTITIES];
} World;

void World_Init(World* w);
EntityId World_CreateEntity(World* w);
void World_DestroyEntity(World* w, EntityId e);
bool World_IsAlive(const World* w, EntityId e);
bool World_HasComponents(const World* w, EntityId e, ComponentMask mask);

// Component add/remove (sets or clears bits in the mask)
static inline void World_AddComponent(World* w, EntityId e, ComponentMask mask) {
    w->masks[e] |= mask;
}
static inline void World_RemoveComponent(World* w, EntityId e, ComponentMask mask) {
    w->masks[e] &= ~mask;
}

// Component accessors (return pointers into parallel arrays)
#ifdef DEBUG
#define ECS_ASSERT_COMPONENT(w, e, comp) \
    assert((e) != ENTITY_NONE && "World_Get" #comp ": entity is ENTITY_NONE"); \
    assert((e) < (EntityId)(w)->count && "World_Get" #comp ": entity out of bounds"); \
    assert((w)->alive[e] && "World_Get" #comp ": entity is dead"); \
    assert(((w)->masks[e] & COMP_##comp) && "World_Get" #comp ": entity missing component")
#else
#define ECS_ASSERT_COMPONENT(w, e, comp) ((void)0)
#endif

static inline CPosition*      World_GetPosition(World* w, EntityId e)  { ECS_ASSERT_COMPONENT(w, e, POSITION);      return &w->positions[e]; }
static inline CStats*         World_GetStats(World* w, EntityId e)     { ECS_ASSERT_COMPONENT(w, e, STATS);         return &w->stats[e]; }
static inline CSpriteAnim*    World_GetSprite(World* w, EntityId e)    { ECS_ASSERT_COMPONENT(w, e, SPRITE_ANIM);   return &w->sprites[e]; }
static inline CFallbackColor* World_GetColor(World* w, EntityId e)     { ECS_ASSERT_COMPONENT(w, e, FALLBACK_COLOR); return &w->colors[e]; }
static inline CAI*            World_GetAI(World* w, EntityId e)        { ECS_ASSERT_COMPONENT(w, e, AI);            return &w->ais[e]; }
static inline CPickup*        World_GetPickup(World* w, EntityId e)    { ECS_ASSERT_COMPONENT(w, e, PICKUP);        return &w->pickups[e]; }
static inline CName*          World_GetName(World* w, EntityId e)      { ECS_ASSERT_COMPONENT(w, e, NAME);          return &w->names[e]; }
static inline CHitFlash*      World_GetHitFlash(World* w, EntityId e)  { ECS_ASSERT_COMPONENT(w, e, HIT_FLASH);     return &w->hitFlashes[e]; }
static inline CAbilities*     World_GetAbilities(World* w, EntityId e) { ECS_ASSERT_COMPONENT(w, e, ABILITIES);     return &w->abilities[e]; }

#endif
