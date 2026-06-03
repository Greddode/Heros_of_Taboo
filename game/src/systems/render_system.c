#include "render_system.h"
#include "map/map_helpers.h"
#include "systems/renderer.h"
#include "data/monster_data.h"

#include <stdio.h>
#include <math.h>


void RenderSystem_DrawMap(const GameWorld* gw) {
    if (!gw || !gw->map) return;

    int tw = gw->map->tileWidth;
    int th = gw->map->tileHeight;

    for (int layer = 0; layer < gw->map->layerCount; layer++) {
        if (!gw->map->layers[layer].visible) continue;
        for (int y = 0; y < gw->map->height; y++) {
            for (int x = 0; x < gw->map->width; x++) {
                unsigned char vis = gw->visibility[y][x];
                if (vis == 0) continue;

                int gid = gw->map->layers[layer].data[y * gw->map->width + x];
                if (gid > 0) {
                    int tsIdx = FindTilesetForGID(gw->map, gid);
                    if (tsIdx >= 0 && gw->tilesetTextures[tsIdx]) {
                        Rectangle src = GetTileSourceRect(gw->map, gid);
                        Vector2 pos = TileToScreen(x, y, tw, th);
                        DrawTextureRec(*gw->tilesetTextures[tsIdx], src, pos, WHITE);
                    }
                }

                if (vis == 2) {
                    Vector2 pos = TileToScreen(x, y, tw, th);
                    DrawRectangle((int)pos.x, (int)pos.y, tw, th, (Color){ 0, 0, 0, 180 });
                }
            }
        }
    }
}

void RenderSystem_DrawMonsters(GameWorld* gw, float monsterT) {
    if (!gw || !gw->map) return;

    int tw = gw->map->tileWidth;
    int th = gw->map->tileHeight;

    for (EntityId e = 1; e < (EntityId)gw->ecs.count; e++) {
        if (!gw->ecs.alive[e]) continue;
        if (World_HasComponents(&gw->ecs, e, COMP_PLAYER_TAG)) continue;
        if (!World_HasComponents(&gw->ecs, e, COMP_POSITION | COMP_STATS)) continue;

        CPosition* p = World_GetPosition(&gw->ecs, e);
        CStats* s = World_GetStats(&gw->ecs, e);
        if (!s->alive) continue;

        if (p->y < 0 || p->y >= gw->map->height || p->x < 0 || p->x >= gw->map->width) continue;
        if (gw->visibility[p->y][p->x] != 1) continue;

        int pixelX, pixelY;
        if (monsterT >= 0.0f && monsterT <= 1.0f) {
            pixelX = (int)(p->prevX * tw + (p->x - p->prevX) * tw * monsterT);
            pixelY = (int)(p->prevY * th + (p->y - p->prevY) * th * monsterT);
        } else {
            pixelX = p->x * tw;
            pixelY = p->y * th;
        }

        CHitFlash* hf = World_GetHitFlash(&gw->ecs, e);

        if (World_HasComponents(&gw->ecs, e, COMP_AI)) {
            CAI* ai = World_GetAI(&gw->ecs, e);
            const MonsterTemplate* tpl = Monster_GetTemplate(ai->type);
            CSpriteAnim* spr = World_GetSprite(&gw->ecs, e);

            bool drewSprite = false;
            if (spr && spr->tex && spr->tex->id > 0 && tpl && tpl->frameCount > 0) {
                float frameW = (float)spr->tex->width / (float)tpl->frameCount;
                float frameH = (float)spr->tex->height;
                Rectangle src = { (float)spr->frame * frameW, 0, frameW, frameH };
                Rectangle dest = { (float)pixelX, (float)pixelY, frameW, frameH };
                Color tint = (hf && hf->timer > 0.0f) ? (Color){ 255, 255, 255, 200 } : WHITE;
                DrawTexturePro(*spr->tex, src, dest, (Vector2){ 0, 0 }, 0, tint);
                drewSprite = true;
            }

            if (!drewSprite && tpl) {
                int pad = tw / 6;
                Color drawColor = (hf && hf->timer > 0.0f) ? WHITE : tpl->color;
                DrawRectangle(pixelX + pad, pixelY + pad, tw - 2 * pad, th - 2 * pad, drawColor);
                int cx = pixelX + tw / 2;
                int cy = pixelY + th / 2;
                DrawCircle(cx - 3, cy - 2, 2, RED);
                DrawCircle(cx + 3, cy - 2, 2, RED);
            }
        }
    }
}

void RenderSystem_World(GameWorld* gw) {
    if (!gw || !gw->map) return;

    int tw = gw->map->tileWidth;
    int th = gw->map->tileHeight;

    BeginMode2D(gw->camera);

    RenderSystem_DrawMap(gw);

    float pd = (gw->animDuration > 0.0f) ? gw->animDuration : MOVE_ANIM_DURATION;
    float md = (gw->monsterAnimDuration > 0.0f) ? gw->monsterAnimDuration : MOVE_ANIM_DURATION;
    float playerT = (gw->animTimer <= 0.0f) ? 1.0f : 1.0f - (gw->animTimer / pd);
    float monsterT = (gw->monsterAnimTimer <= 0.0f) ? 1.0f : 1.0f - (gw->monsterAnimTimer / md);

    RenderSystem_DrawMonsters(gw, monsterT);

    // Pickup rendering (ECS entities with COMP_POSITION | COMP_PICKUP)
    for (EntityId e = 1; e < (EntityId)gw->ecs.count; e++) {
        if (!gw->ecs.alive[e]) continue;
        if (!World_HasComponents(&gw->ecs, e, COMP_POSITION | COMP_PICKUP | COMP_SPRITE_ANIM)) continue;

        CPosition* p = World_GetPosition(&gw->ecs, e);
        CPickup* pk = World_GetPickup(&gw->ecs, e);
        CSpriteAnim* spr = World_GetSprite(&gw->ecs, e);

        if (pk->quantity <= 0) continue;

        if (p->y < 0 || p->y >= gw->map->height || p->x < 0 || p->x >= gw->map->width) continue;
        if (gw->visibility[p->y][p->x] != 1) continue;

        int px = p->x * tw;
        int py = p->y * th;

        if (spr->tex && spr->tex->id > 0) {
            Rectangle src = { (float)spr->frame * 16, (float)spr->row * 16, 16, 16 };
            Rectangle dest = { (float)px, (float)py, (float)tw, (float)th };
            DrawTexturePro(*spr->tex, src, dest, (Vector2){ 0 }, 0, WHITE);
        } else {
            DrawRectangle(px + 2, py + 2, tw - 4, th - 4, pk->isEquip ? (Color){ 200, 150, 50, 255 } : (Color){ 50, 200, 100, 255 });
        }

        if (pk->quantity > 1) {
            char qty[8];
            snprintf(qty, sizeof(qty), "x%d", pk->quantity);
            DrawText(qty, px + 2, py + 2, 10, YELLOW);
        }
    }

    // Player rendering
    if (gw->playerEntity != ENTITY_NONE && World_GetStats(&gw->ecs, gw->playerEntity)->alive) {
        CPosition* pp = World_GetPosition(&gw->ecs, gw->playerEntity);
        CHitFlash* phf = World_GetHitFlash(&gw->ecs, gw->playerEntity);
        CSpriteAnim* spr = World_GetSprite(&gw->ecs, gw->playerEntity);

        float px, py;
        if (playerT >= 0.0f && playerT <= 1.0f) {
            px = (float)(pp->prevX * tw) * (1.0f - playerT) + (float)(pp->x * tw) * playerT;
            py = (float)(pp->prevY * th) * (1.0f - playerT) + (float)(pp->y * th) * playerT;
        } else {
            px = (float)(pp->x * tw);
            py = (float)(pp->y * th);
        }

        if (spr && spr->tex && spr->tex->id > 0 && spr->frameCount > 0) {
            float frameW = (float)spr->tex->width / (float)spr->frameCount;
            float frameH = (float)spr->tex->height;
            Rectangle src = {
                (float)spr->frame * frameW,
                (float)spr->row * frameH,
                frameW, frameH
            };
            Rectangle dest = { px, py, (float)tw, (float)th };
            Color tint = (phf && phf->timer > 0.0f) ? (Color){ 255, 255, 255, 200 } : WHITE;
            DrawTexturePro(*spr->tex, src, dest, (Vector2){ 0, 0 }, 0, tint);
        } else {
            int pad = tw / 6;
            Color playerColor = (phf && phf->timer > 0.0f) ? WHITE : (Color){ 50, 200, 255, 255 };
            DrawRectangle((int)px + pad, (int)py + pad, tw - 2 * pad, th - 2 * pad, playerColor);
            int cx = (int)px + tw / 2;
            int cy = (int)py + th / 2;
            DrawCircle(cx - 3, cy - 2, 2, WHITE);
            DrawCircle(cx + 3, cy - 2, 2, WHITE);
        }
    }

    // Projectile
    if (gw->projectile.active && gw->projectileDuration > 0.0f) {
        float pt = (gw->projectileTimer <= 0.0f) ? 1.0f : 1.0f - (gw->projectileTimer / gw->projectileDuration);
        float cx = gw->projectile.sx + (gw->projectile.ex - gw->projectile.sx) * pt;
        float cy = gw->projectile.sy + (gw->projectile.ey - gw->projectile.sy) * pt;

        float sx = gw->projectile.sx, sy = gw->projectile.sy;
        float ddx = cx - sx, ddy = cy - sy;
        float len = sqrtf(ddx * ddx + ddy * ddy);
        if (len > 4.0f) {
            float nx = ddx / len, ny = ddy / len;
            float headLen = 12.0f;
            float shaftEnd = len - headLen;
            if (shaftEnd < 0) shaftEnd = 0;
            float sx2 = sx + nx * shaftEnd, sy2 = sy + ny * shaftEnd;
            DrawLineEx((Vector2){ sx, sy }, (Vector2){ sx2, sy2 }, 3.0f, gw->projectile.color);
            float tipX = sx + nx * len, tipY = sy + ny * len;
            float ppx = -ny, ppy = nx;
            float headW = 6.0f;
            Vector2 v1 = { tipX, tipY };
            Vector2 v2 = { sx2 + ppx * headW, sy2 + ppy * headW };
            Vector2 v3 = { sx2 - ppx * headW, sy2 - ppy * headW };
            DrawTriangle(v1, v2, v3, gw->projectile.color);
        } else {
            DrawRectangle((int)sx - 2, (int)sy - 2, 4, 4, gw->projectile.color);
        }
    }

    EndMode2D();
}

void RenderSystem_HUD(GameWorld* gw) {
    if (!gw) return;

    float scale = GetUIScale();
    int panelX = (int)(10 * scale);
    int panelW = (int)(260 * scale);
    int barW = panelW - (int)(20 * scale);
    int barH = (int)(16 * scale);
    int textY = 0;

    if (gw->playerEntity == ENTITY_NONE) return;
    CStats* ps = World_GetStats(&gw->ecs, gw->playerEntity);

    char levelText[64];
    snprintf(levelText, sizeof(levelText), "Lv %d", ps->level);
    int panelH = (int)(100 * scale);
    int panelY = GetScreenHeight() - (int)(10 * scale) - panelH;
    DrawRectangle(panelX - 4, panelY, panelW, panelH, (Color){ 0, 0, 0, 180 });
    textY = panelY + (int)(4 * scale);
    DrawText(levelText, panelX, textY, (int)(18 * scale), BLACK);

    // HP bar
    textY += (int)(24 * scale);
    float hpRatio = (ps->maxHp > 0) ? (float)ps->hp / (float)ps->maxHp : 0;
    if (hpRatio < 0) hpRatio = 0;
    DrawRectangle(panelX, textY, barW, barH, (Color){ 60, 0, 0, 255 });
    DrawRectangle(panelX, textY, (int)(barW * hpRatio), barH, RED);
    char hpText[64];
    snprintf(hpText, sizeof(hpText), "HP: %d/%d", ps->hp, ps->maxHp);
    DrawText(hpText, panelX + (int)(4 * scale), textY + (int)(1 * scale), (int)(14 * scale), WHITE);

    // EXP bar
    textY += barH + (int)(6 * scale);
    float expRatio = (ps->expToNext > 0) ? (float)ps->exp / (float)ps->expToNext : 0;
    if (expRatio < 0) expRatio = 0;
    DrawRectangle(panelX, textY, barW, barH, (Color){ 0, 0, 60, 255 });
    DrawRectangle(panelX, textY, (int)(barW * expRatio), barH, (Color){ 80, 80, 255, 255 });
    char expText[64];
    snprintf(expText, sizeof(expText), "EXP: %d/%d", ps->exp, ps->expToNext);
    DrawText(expText, panelX + (int)(4 * scale), textY + (int)(1 * scale), (int)(14 * scale), WHITE);

    // Floor info
    textY += barH + (int)(6 * scale);
    char infoText[64];
    snprintf(infoText, sizeof(infoText), "Floor: %d/%d", gw->currentFloor, gw->maxFloors);
    DrawText(infoText, panelX, textY, (int)(14 * scale), BLACK);

    // State text
    const char* stateText = "";
    if (gw->state == STATE_GAME_OVER) stateText = "GAME OVER - Hold Shift+R to restart";
    else if (gw->state == STATE_WIN) stateText = "YOU WIN! - Hold Shift+R to restart";
    else if (gw->state == STATE_PLAYER_TURN) stateText = "Your turn";
    else if (gw->state == STATE_ENEMY_TURN) stateText = "Enemy turn...";

    if (stateText[0]) {
        int textWidth = MeasureText(stateText, (int)(20 * scale));
        DrawText(stateText, (GetScreenWidth() - textWidth) / 2, GetScreenHeight() - (int)(40 * scale), (int)(20 * scale), YELLOW);
    }

    // Level-up overlay
    if (gw->levelUpTimer > 0.0f) {
        float alpha = gw->levelUpTimer / 3.0f;
        float textScale = 1.0f + (1.0f - alpha) * 0.3f;
        int fontSize = (int)(48 * scale * textScale);
        int subSize = (int)(24 * scale * textScale);
        const char* title = "LEVEL UP!";
        const char* sub = "+2 Stat Points!";
        int tw2 = MeasureText(title, fontSize);
        int sw2 = MeasureText(sub, subSize);
        int cx2 = GetScreenWidth() / 2;
        int cy2 = GetScreenHeight() / 2;

        int glowW = (int)(fmaxf(tw2, sw2) * 1.5f);
        int glowH = (int)((fontSize + subSize + 40) * 1.5f);
        Color bg = { 0, 0, 0, (unsigned char)(180 * alpha) };
        DrawRectangle(cx2 - glowW / 2, cy2 - glowH / 2, glowW, glowH, bg);

        Color titleColor = { 255, 215, 0, (unsigned char)(255 * alpha) };
        DrawText(title, cx2 - tw2 / 2, cy2 - fontSize - (int)(8 * scale), fontSize, titleColor);
        Color subColor = { 255, 255, 255, (unsigned char)(255 * alpha) };
        DrawText(sub, cx2 - sw2 / 2, cy2 + (int)(8 * scale), subSize, subColor);
    }
}
