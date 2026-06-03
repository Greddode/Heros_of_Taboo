#include "world.h"
#include <string.h>
#include <stdio.h>

void GameWorld_Init(GameWorld* gw) {
    memset(gw, 0, sizeof(GameWorld));
    World_Init(&gw->ecs);
    gw->playerEntity = ENTITY_NONE;
    gw->selectedMonsterEntity = ENTITY_NONE;
}

void DamageNumber_Spawn(DamageNumberPool* pool, int value, int tileX, int tileY, int tw, int th, Color color) {
    if (!pool) return;
    for (int i = 0; i < MAX_DAMAGE_NUMBERS; i++) {
        if (!pool->entries[i].active) {
            snprintf(pool->entries[i].text, sizeof(pool->entries[i].text), "%d", value);
            pool->entries[i].pos.x = (float)(tileX * tw + tw / 2);
            pool->entries[i].pos.y = (float)(tileY * th);
            pool->entries[i].timer = DAMAGE_NUMBER_LIFETIME;
            pool->entries[i].color = color;
            pool->entries[i].active = true;
            return;
        }
    }
}

void DamageNumber_UpdateAll(DamageNumberPool* pool, float dt) {
    if (!pool) return;
    for (int i = 0; i < MAX_DAMAGE_NUMBERS; i++) {
        if (!pool->entries[i].active) continue;
        pool->entries[i].timer -= dt;
        pool->entries[i].pos.y -= DAMAGE_NUMBER_FLOAT_SPEED * dt;
        if (pool->entries[i].timer <= 0.0f)
            pool->entries[i].active = false;
    }
}
