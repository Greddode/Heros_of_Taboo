#include "player_system.h"
#include "systems/player.h"
#include "resources.h"
#include "game_balance.h"
#include <stdio.h>

void PlayerSystem_Spawn(GameWorld* gw) {
    EntityId e = World_CreateEntity(&gw->ecs);
    if (e == ENTITY_NONE) return;
    gw->playerEntity = e;

    World_AddComponent(&gw->ecs, e, COMP_POSITION);
    CPosition* pos = World_GetPosition(&gw->ecs, e);
    pos->x = PLAYER_SPAWN_X;
    pos->y = PLAYER_SPAWN_Y;
    pos->prevX = PLAYER_SPAWN_X;
    pos->prevY = PLAYER_SPAWN_Y;
    pos->facingDir = DIR_DOWN;

    World_AddComponent(&gw->ecs, e, COMP_STATS);
    CStats* s = World_GetStats(&gw->ecs, e);
    s->str = PLAYER_BASE_STR;
    s->dex = PLAYER_BASE_DEX;
    s->intel = PLAYER_BASE_INT;
    s->con = PLAYER_BASE_CON;
    s->lck = PLAYER_BASE_LCK;
    s->statPoints = 0;
    s->attack = PLAYER_BASE_ATTACK;
    s->defense = PLAYER_BASE_DEFENSE;
    s->level = PLAYER_BASE_LEVEL;
    s->alive = true;
    s->exp = 0;
    s->expToNext = ExpForLevel(PLAYER_BASE_LEVEL);
    s->expValue = 0;
    s->maxHp = calc_max_hp(s->con);
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

    World_AddComponent(&gw->ecs, e, COMP_ABILITIES);
    CAbilities* a = World_GetAbilities(&gw->ecs, e);
    a->abilities[0] = ABILITY_PUNCH;
    a->count = 1;
    a->maxMp = MP_BASE + s->intel * MP_PER_INT;
    a->mp = a->maxMp;
}
