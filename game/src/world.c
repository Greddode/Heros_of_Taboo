#include "world.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

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
    TraceLog(LOG_WARNING, "DamageNumber_Spawn: pool full [value=%d at (%d,%d)]", value, tileX, tileY);
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

void FloatMsg_Spawn(GameWorld* gw, int tileX, int tileY, Color color, const char* fmt, ...) {
    if (!gw || !fmt) return;
    for (int i = 0; i < MAX_FLOAT_MSGS; i++) {
        if (!gw->floatMsgs.entries[i].active) {
            va_list args;
            va_start(args, fmt);
            vsnprintf(gw->floatMsgs.entries[i].text, sizeof(gw->floatMsgs.entries[i].text), fmt, args);
            va_end(args);
            gw->floatMsgs.entries[i].worldX = (float)(tileX * gw->map->tileWidth + gw->map->tileWidth / 2);
            gw->floatMsgs.entries[i].worldY = (float)(tileY * gw->map->tileHeight);
            gw->floatMsgs.entries[i].velY = FLOAT_MSG_VEL_Y;
            gw->floatMsgs.entries[i].alpha = 1.0f;
            gw->floatMsgs.entries[i].lifetime = FLOAT_MSG_LIFETIME;
            gw->floatMsgs.entries[i].color = color;
            gw->floatMsgs.entries[i].active = true;
            return;
        }
    }
    TraceLog(LOG_WARNING, "FloatMsg_Spawn: pool full [at (%d,%d)]", tileX, tileY);
}

void FloatMsg_UpdateAll(GameWorld* gw, float dt) {
    if (!gw) return;
    for (int i = 0; i < MAX_FLOAT_MSGS; i++) {
        if (!gw->floatMsgs.entries[i].active) continue;
        gw->floatMsgs.entries[i].lifetime -= dt;
        gw->floatMsgs.entries[i].worldY += gw->floatMsgs.entries[i].velY * dt;
        gw->floatMsgs.entries[i].alpha = gw->floatMsgs.entries[i].lifetime / FLOAT_MSG_LIFETIME;
        if (gw->floatMsgs.entries[i].alpha < 0.0f) gw->floatMsgs.entries[i].alpha = 0.0f;
        if (gw->floatMsgs.entries[i].lifetime <= 0.0f)
            gw->floatMsgs.entries[i].active = false;
    }
}
