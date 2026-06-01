#include "player_system.h"
#include "systems/player.h"
#include "resources.h"
#include <stdio.h>

void PlayerSystem_Spawn(GameWorld* gw) {
    EntityId e = World_CreateEntity(&gw->ecs);
    if (e == ENTITY_NONE) return;
    gw->playerEntity = e;

    World_AddComponent(&gw->ecs, e, COMP_POSITION);
    CPosition* pos = World_GetPosition(&gw->ecs, e);
    pos->x = 1;
    pos->y = 1;
    pos->prevX = 1;
    pos->prevY = 1;
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
    s->expToNext = ExpForLevel(1);
    s->expValue = 0;
    s->maxHp = 30 + s->con * 5;
    s->hp = s->maxHp;

    World_AddComponent(&gw->ecs, e, COMP_SPRITE_ANIM);
    CSpriteAnim* spr = World_GetSprite(&gw->ecs, e);
    spr->tex = Resources_LoadTexture("resources/sprites/player.png");
    spr->row = 6;
    spr->frame = 0;
    spr->frameCount = 4;
    spr->animTimer = 0;
    spr->animSpeed = 0.5f;

    World_AddComponent(&gw->ecs, e, COMP_FALLBACK_COLOR);
    World_GetColor(&gw->ecs, e)->color = (Color){ 50, 200, 255, 255 };

    World_AddComponent(&gw->ecs, e, COMP_NAME);
    CName* n = World_GetName(&gw->ecs, e);
    snprintf(n->name, sizeof(n->name), "Player");

    World_AddComponent(&gw->ecs, e, COMP_PLAYER_TAG);

    World_AddComponent(&gw->ecs, e, COMP_HIT_FLASH);
    CHitFlash* hf = World_GetHitFlash(&gw->ecs, e);
    hf->timer = 0;
}
