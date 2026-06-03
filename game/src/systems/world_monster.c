#include "world_monster.h"
#include "spatial_hash.h"
#include <stddef.h>

EntityId World_FindMonsterAt(GameWorld* gw, int x, int y, EntityId exclude) {
    if (!gw) return ENTITY_NONE;
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return ENTITY_NONE;
    EntityId e = gw->monsterGrid[y][x];
    if (e == ENTITY_NONE || e == exclude) return ENTITY_NONE;
    if (!gw->ecs.alive[e]) return ENTITY_NONE;
    if (!World_HasComponents(&gw->ecs, e, COMP_POSITION | COMP_STATS | COMP_AI)) return ENTITY_NONE;
    if (World_HasComponents(&gw->ecs, e, COMP_PLAYER_TAG)) return ENTITY_NONE;
    if (World_GetStats(&gw->ecs, e)->alive) return e;
    return ENTITY_NONE;
}

int World_CountAliveMonsters(GameWorld* gw) {
    if (!gw) return 0;
    return gw->aliveMonsterCount;
}

bool World_AreAllMonstersDead(GameWorld* gw) {
    if (!gw) return true;
    return gw->aliveMonsterCount <= 0;
}

void World_UpdateMonsterAnimations(GameWorld* gw, float dt) {
    if (!gw) return;
    for (EntityId e = 1; e < (EntityId)gw->ecs.count; e++) {
        if (!gw->ecs.alive[e]) continue;
        if (!World_HasComponents(&gw->ecs, e, COMP_STATS | COMP_SPRITE_ANIM | COMP_AI)) continue;
        if (World_HasComponents(&gw->ecs, e, COMP_PLAYER_TAG)) continue;

        CStats* s = World_GetStats(&gw->ecs, e);
        if (!s->alive) continue;

        CHitFlash* hf = World_HasComponents(&gw->ecs, e, COMP_HIT_FLASH)
            ? World_GetHitFlash(&gw->ecs, e) : NULL;
        if (hf && hf->timer > 0.0f) {
            hf->timer -= dt;
            if (hf->timer < 0.0f) hf->timer = 0.0f;
        }

        CSpriteAnim* spr = World_GetSprite(&gw->ecs, e);
        if (!spr || spr->frameCount <= 1) continue;

        spr->animTimer += dt;
        if (spr->animTimer >= spr->animSpeed) {
            spr->animTimer -= spr->animSpeed;
            spr->frame = (spr->frame + 1) % spr->frameCount;
        }
    }
}
