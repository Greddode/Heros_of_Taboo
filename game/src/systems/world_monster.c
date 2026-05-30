#include "world_monster.h"
#include <stddef.h>

EntityId World_FindMonsterAt(GameWorld* gw, int x, int y, EntityId exclude) {
    if (!gw) return ENTITY_NONE;
    for (EntityId e = 1; e < (EntityId)gw->ecs.count; e++) {
        if (!gw->ecs.alive[e]) continue;
        if (e == exclude) continue;
        if (!World_HasComponents(&gw->ecs, e, COMP_POSITION | COMP_STATS | COMP_AI)) continue;
        if (World_HasComponents(&gw->ecs, e, COMP_PLAYER_TAG)) continue;
        CPosition* p = World_GetPosition(&gw->ecs, e);
        CStats* s = World_GetStats(&gw->ecs, e);
        if (s->alive && p->x == x && p->y == y) return e;
    }
    return ENTITY_NONE;
}

int World_CountAliveMonsters(GameWorld* gw) {
    if (!gw) return 0;
    int count = 0;
    for (EntityId e = 1; e < (EntityId)gw->ecs.count; e++) {
        if (!gw->ecs.alive[e]) continue;
        if (!World_HasComponents(&gw->ecs, e, COMP_STATS | COMP_AI)) continue;
        if (World_HasComponents(&gw->ecs, e, COMP_PLAYER_TAG)) continue;
        if (World_GetStats(&gw->ecs, e)->alive) count++;
    }
    return count;
}

bool World_AreAllMonstersDead(GameWorld* gw) {
    return World_CountAliveMonsters(gw) == 0;
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
