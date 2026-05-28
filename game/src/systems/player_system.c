#include "player_system.h"
#include "core/audio.h"
#include "ui/combat_log.h"
#include "resources.h"
#include <stdio.h>

static int ExpForLevelLocal(int level) {
    return 20 + level * 10;
}

void PlayerSystem_Spawn(GameWorld* gw) {
    EntityId e = World_CreateEntity(&gw->ecs);
    if (e == ENTITY_NONE) return;
    gw->playerEntity = e;

    World_AddComponent(&gw->ecs, e, COMP_POSITION);
    CPosition* pos = World_GetPosition(&gw->ecs, e);
    pos->x = 0;
    pos->y = 0;
    pos->prevX = 0;
    pos->prevY = 0;
    pos->facingDir = DIR_DOWN;

    World_AddComponent(&gw->ecs, e, COMP_STATS);
    CStats* s = World_GetStats(&gw->ecs, e);
    s->str = 3;
    s->dex = 3;
    s->intel = 3;
    s->con = 3;
    s->lck = 2;
    s->statPoints = 0;
    s->attack = 5;
    s->defense = 1;
    s->level = 1;
    s->alive = true;
    s->exp = 0;
    s->expToNext = ExpForLevelLocal(1);
    s->expValue = 0;
    s->maxHp = 30 + s->con * 5;
    s->hp = s->maxHp;

    World_AddComponent(&gw->ecs, e, COMP_SPRITE_ANIM);
    CSpriteAnim* spr = World_GetSprite(&gw->ecs, e);
    spr->tex = Resources_LoadTexture("resources/sprites/player.png");
    spr->row = 0;
    spr->frame = 0;
    spr->frameCount = 4;
    spr->animTimer = 0;
    spr->animSpeed = 0.15f;

    World_AddComponent(&gw->ecs, e, COMP_NAME);
    CName* n = World_GetName(&gw->ecs, e);
    snprintf(n->name, sizeof(n->name), "Player");

    World_AddComponent(&gw->ecs, e, COMP_PLAYER_TAG);

    World_AddComponent(&gw->ecs, e, COMP_HIT_FLASH);
    CHitFlash* hf = World_GetHitFlash(&gw->ecs, e);
    hf->timer = 0;
}

void PlayerSystem_GainExperience(GameWorld* gw, int amount) {
    if (gw->playerEntity == ENTITY_NONE) return;
    CStats* s = World_GetStats(&gw->ecs, gw->playerEntity);
    s->exp += amount;
    TraceLog(LOG_INFO, "Gained %d exp (total: %d, next: %d)", amount, s->exp, s->expToNext);
    while (s->exp >= s->expToNext) {
        s->exp -= s->expToNext;
        s->level++;
        s->expToNext = ExpForLevelLocal(s->level);
        s->statPoints += 2;
        gw->levelUpTimer = 3.0f;
        PlayLevelUpSound();
        TraceLog(LOG_INFO, "Level up! Now level %d (%d stat points available)", s->level, s->statPoints);
        CombatLog_Add(&gw->combatLog, BLACK, "Level %d! +2 stat points to allocate!", s->level);
    }
}

bool PlayerSystem_AllocateStatPoint(GameWorld* gw, int statIdx) {
    if (gw->playerEntity == ENTITY_NONE) return false;
    CStats* s = World_GetStats(&gw->ecs, gw->playerEntity);
    if (s->statPoints <= 0) return false;
    switch (statIdx) {
        case 0: s->str++;   break;
        case 1: s->dex++;   break;
        case 2: s->intel++; break;
        case 3: s->con++;   break;
        case 4: s->lck++;   break;
        default: return false;
    }
    s->statPoints--;
    s->maxHp = 30 + s->con * 5;
    if (s->hp > s->maxHp) s->hp = s->maxHp;
    return true;
}
