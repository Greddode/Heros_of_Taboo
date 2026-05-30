#include "game.h"
#include "systems/render_system.h"
#include "systems/spawner_system.h"
#include "ui/combat_log.h"
#include "ui/inspector.h"
#include "ui/inventory_ui.h"
#include "resources.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void Draw9Slice(Texture2D tex, Rectangle dest, int l, int t, int r, int b) {
    float sw = (float)tex.width, sh = (float)tex.height;
    float x = dest.x, y = dest.y, w = dest.width, h = dest.height;
    float mw = sw - l - r, mh = sh - t - b;
    float dw = w - l - r, dh = h - t - b;

    DrawTexturePro(tex, (Rectangle){0,0,l,t}, (Rectangle){x,y,l,t}, (Vector2){0},0,WHITE);
    DrawTexturePro(tex, (Rectangle){sw-r,0,r,t}, (Rectangle){x+w-r,y,r,t}, (Vector2){0},0,WHITE);
    DrawTexturePro(tex, (Rectangle){0,sh-b,l,b}, (Rectangle){x,y+h-b,l,b}, (Vector2){0},0,WHITE);
    DrawTexturePro(tex, (Rectangle){sw-r,sh-b,r,b}, (Rectangle){x+w-r,y+h-b,r,b}, (Vector2){0},0,WHITE);
    DrawTexturePro(tex, (Rectangle){l,0,mw,t}, (Rectangle){x+l,y,dw,t}, (Vector2){0},0,WHITE);
    DrawTexturePro(tex, (Rectangle){l,sh-b,mw,b}, (Rectangle){x+l,y+h-b,dw,b}, (Vector2){0},0,WHITE);
    DrawTexturePro(tex, (Rectangle){0,t,l,mh}, (Rectangle){x,y+l,l,dh}, (Vector2){0},0,WHITE);
    DrawTexturePro(tex, (Rectangle){sw-r,t,r,mh}, (Rectangle){x+w-r,y+l,r,dh}, (Vector2){0},0,WHITE);
    DrawTexturePro(tex, (Rectangle){l,t,mw,mh}, (Rectangle){x+l,y+l,dw,dh}, (Vector2){0},0,WHITE);
}

void RenderGame(const GameWorld* game, const InventoryUIState* ui) {
    if (!game || !game->map) return;

    int tw = game->map->tileWidth;
    int th = game->map->tileHeight;

    BeginMode2D(game->camera);

    RenderSystem_DrawMap(game);

    // Pickups (ECS)
    {
        bool renderedEquipTiles[MAP_HEIGHT][MAP_WIDTH] = { false };

        for (EntityId eid = 1; eid < (EntityId)game->ecs.count; eid++) {
            if (!game->ecs.alive[eid]) continue;
            if (!World_HasComponents(&((GameWorld*)game)->ecs, eid, COMP_POSITION | COMP_PICKUP)) continue;

            CPosition* pos = World_GetPosition(&((GameWorld*)game)->ecs, eid);
            CPickup* pk = World_GetPickup(&((GameWorld*)game)->ecs, eid);
            if (pk->quantity <= 0) continue;
            if (pos->y < 0 || pos->y >= game->map->height || pos->x < 0 || pos->x >= game->map->width) continue;
            if (game->visibility[pos->y][pos->x] != 1) continue;

            if (!pk->isEquip) {
                Texture2D* pTex = Inventory_LoadPotionTexture(pk->itemType);
                if (pTex && pTex->id > 0) {
                    Vector2 hpos = TileToScreen(pos->x, pos->y, tw, th);
                    Rectangle dest = { hpos.x, hpos.y, (float)tw, (float)th };
                    DrawTexturePro(*pTex,
                                   (Rectangle){ 0, 0, (float)pTex->width, (float)pTex->height },
                                   dest, (Vector2){ 0, 0 }, 0, WHITE);
                    if (pk->quantity > 1) {
                        char qty[8];
                        snprintf(qty, sizeof(qty), "x%d", pk->quantity);
                        DrawText(qty, hpos.x + 2, hpos.y + 2, 10, YELLOW);
                    }
                }
            } else if (!renderedEquipTiles[pos->y][pos->x]) {
                renderedEquipTiles[pos->y][pos->x] = true;
                int ex = pos->x;
                int ey = pos->y;
                int entityCount = 0;
                int stackQty = 0;
                EquipType singleType = EQUIP_NONE;
                for (EntityId ej = 1; ej < (EntityId)game->ecs.count; ej++) {
                    if (!game->ecs.alive[ej]) continue;
                    if (!World_HasComponents(&((GameWorld*)game)->ecs, ej, COMP_POSITION | COMP_PICKUP)) continue;
                    CPosition* pj = World_GetPosition(&((GameWorld*)game)->ecs, ej);
                    CPickup* pkj = World_GetPickup(&((GameWorld*)game)->ecs, ej);
                    if (pkj->quantity <= 0 || !pkj->isEquip || pj->x != ex || pj->y != ey) continue;
                    entityCount++;
                    stackQty += pkj->quantity;
                    singleType = pkj->equipType;
                }

                Vector2 epos = TileToScreen(ex, ey, tw, th);
                if (entityCount > 1) {
                    Texture2D* lootTex = Resources_LoadTexture("resources/sprites/items/loot.png");
                    if (lootTex && lootTex->id > 0) {
                        Rectangle dest = { epos.x, epos.y, (float)tw, (float)th };
                        DrawTexturePro(*lootTex,
                                       (Rectangle){ 0, 0, (float)lootTex->width, (float)lootTex->height },
                                       dest, (Vector2){ 0, 0 }, 0, WHITE);
                    }
                    char qty[8];
                    snprintf(qty, sizeof(qty), "x%d", entityCount);
                    DrawText(qty, epos.x + 2, epos.y + 2, 10, YELLOW);
                } else {
                    Texture2D* tex = Inventory_LoadEquipTexture(singleType);
                    if (tex && tex->id > 0) {
                        Rectangle src = { 0, 0, (float)tex->width, (float)tex->height };
                        Rectangle dest = { epos.x, epos.y, (float)tw, (float)th };
                        DrawTexturePro(*tex, src, dest, (Vector2){ 0 }, 0, WHITE);
                    }
                    if (stackQty > 1) {
                        char buf[8];
                        snprintf(buf, sizeof(buf), "x%d", stackQty);
                        DrawText(buf, epos.x + 2, epos.y + 2, 10, YELLOW);
                    }
                }
            }
        }
    }

    // Interpolation factors
    float pd = (game->animDuration > 0.0f) ? game->animDuration : MOVE_ANIM_DURATION;
    float md = (game->monsterAnimDuration > 0.0f) ? game->monsterAnimDuration : MOVE_ANIM_DURATION;
    float playerT = (game->animTimer <= 0.0f) ? 1.0f : 1.0f - (game->animTimer / pd);
    float monsterT = (game->monsterAnimTimer <= 0.0f) ? 1.0f : 1.0f - (game->monsterAnimTimer / md);
    RenderSystem_DrawMonsters((GameWorld*)game, monsterT);

    // Player rendering
    if (game->playerEntity != ENTITY_NONE) {
        World* w = &((GameWorld*)game)->ecs;
        EntityId pe = game->playerEntity;
        const CStats* pstats = World_GetStats(w, pe);
        const CPosition* ppos = World_GetPosition(w, pe);
        bool hasSprite = World_HasComponents(w, pe, COMP_SPRITE_ANIM);
        bool hasColor = World_HasComponents(w, pe, COMP_FALLBACK_COLOR);
        bool hasHitFlash = World_HasComponents(w, pe, COMP_HIT_FLASH);
        float hitFlashTimer = hasHitFlash ? World_GetHitFlash(w, pe)->timer : 0.0f;

        if (pstats->alive) {
            float px = (float)(ppos->prevX * tw) * (1.0f - playerT) + (float)(ppos->x * tw) * playerT;
            float py = (float)(ppos->prevY * th) * (1.0f - playerT) + (float)(ppos->y * th) * playerT;

            if (hasSprite && World_GetSprite(w, pe)->tex && World_GetSprite(w, pe)->tex->id > 0) {
                const CSpriteAnim* sa = World_GetSprite(w, pe);
                int frameCount = 4;
                float frameW = (float)sa->tex->width / (float)frameCount;
                float frameH = (float)sa->tex->height;
                Rectangle src = {
                    (float)(sa->frame * frameW),
                    0,
                    frameW, frameH
                };
                Rectangle dest = { px, py, (float)tw, (float)th };
                Color tint = (hitFlashTimer > 0.0f) ? (Color){ 255, 255, 255, 200 } : WHITE;
                DrawTexturePro(*sa->tex, src, dest, (Vector2){ 0, 0 }, 0, tint);
            } else {
                int pad = tw / 6;
                Color playerColor = (hitFlashTimer > 0.0f) ? WHITE
                    : (hasColor ? World_GetColor(w, pe)->color : (Color){ 50, 200, 255, 255 });
                DrawRectangle(px + pad, py + pad, tw - 2*pad, th - 2*pad, playerColor);

                int cx = px + tw / 2;
                int cy = py + th / 2;
                DrawCircle(cx - 3, cy - 2, 2, WHITE);
                DrawCircle(cx + 3, cy - 2, 2, WHITE);

                if (ppos->facingDir == DIR_RIGHT) {
                    DrawCircle(cx + 5, cy + 4, 1, WHITE);
                } else if (ppos->facingDir == DIR_LEFT) {
                    DrawCircle(cx - 5, cy + 4, 1, WHITE);
                } else if (ppos->facingDir == DIR_UP) {
                    DrawCircle(cx, cy - 5, 1, WHITE);
                } else {
                    DrawCircle(cx, cy + 5, 1, WHITE);
                }
            }

            // Direction indicator
            if (game->state == STATE_PLAYER_TURN) {
                int dx = ppos->x, dy = ppos->y;
                switch (ppos->facingDir) {
                    case DIR_UP:    dy--; break;
                    case DIR_DOWN:  dy++; break;
                    case DIR_LEFT:  dx--; break;
                    case DIR_RIGHT: dx++; break;
                    default: break;
                }
                if (dx >= 0 && dx < game->map->width && dy >= 0 && dy < game->map->height) {
                    Vector2 dpos = TileToScreen(dx, dy, tw, th);
                    DrawRectangle(dpos.x, dpos.y, tw, th, (Color){ 255, 50, 50, 100 });
                }
            }
        }
    }

    // Projectile rendering
    if (game->projectile.active && game->projectileDuration > 0.0f) {
        float pt = (game->projectileTimer <= 0.0f) ? 1.0f : 1.0f - (game->projectileTimer / game->projectileDuration);
        float cx = game->projectile.sx + (game->projectile.ex - game->projectile.sx) * pt;
        float cy = game->projectile.sy + (game->projectile.ey - game->projectile.sy) * pt;

        Texture2D* magicTex = Resources_LoadTexture("resources/tilesets/magic_attacks.png");
        if (game->projectile.attackType == ATTACK_MAGIC &&
            magicTex && magicTex->id > 0 && game->projectile.animFrameCount > 0) {
            int x0 = game->projectile.tileSX, y0 = game->projectile.tileSY;
            int x1 = game->projectile.tileEX, y1 = game->projectile.tileEY;
            int mapTw = game->map->tileWidth, mapTh = game->map->tileHeight;
            int totalDist = abs(x1 - x0) + abs(y1 - y0);
            int cols = 10, tileSize = 64;
            int animLen = game->projectile.animFrameCount;

            int bdx = abs(x1 - x0), bdy = abs(y1 - y0);
            int sxDir = (x0 < x1) ? 1 : -1, syDir = (y0 < y1) ? 1 : -1;
            int err = bdx - bdy;
            int x = x0, y = y0;
            while (1) {
                int tx = x, ty = y;

                int d = abs(tx - x0) + abs(ty - y0);
                float wavePos = pt * (totalDist + animLen);
                float waveOffset = wavePos - d;
                if (waveOffset >= 0) {
                    int frame = (int)waveOffset;
                    if (frame >= animLen) frame = animLen - 1;
                    int fIndex = game->projectile.startFrame + frame;
                    Rectangle src = {
                        (float)((fIndex % cols) * tileSize),
                        (float)((fIndex / cols) * tileSize),
                        (float)tileSize, (float)tileSize
                    };
                    if (tx >= 0 && tx < game->map->width && ty >= 0 && ty < game->map->height) {
                        Rectangle dest = { (float)(tx * mapTw), (float)(ty * mapTh), (float)mapTw, (float)mapTh };
                        DrawTexturePro(*magicTex, src, dest, (Vector2){ 0 }, 0, WHITE);
                    }
                }

                if (tx == x1 && ty == y1) break;
                int e2 = 2 * err;
                if (e2 > -bdy) { err -= bdy; x += sxDir; }
                if (e2 < bdx)  { err += bdx; y += syDir; }
            }
        } else {
            float sx = game->projectile.sx, sy = game->projectile.sy;
            float dx = cx - sx, dy = cy - sy;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 4.0f) {
                float nx = dx / len, ny = dy / len;
                float headLen = 12.0f;
                float shaftEnd = len - headLen;
                if (shaftEnd < 0) shaftEnd = 0;
                float sx2 = sx + nx * shaftEnd, sy2 = sy + ny * shaftEnd;
                DrawLineEx((Vector2){ sx, sy }, (Vector2){ sx2, sy2 }, 3.0f, game->projectile.color);
                float tipX = sx + nx * len, tipY = sy + ny * len;
                float perpX = -ny, perpY = nx;
                float headW = 6.0f;
                Vector2 v1 = { tipX, tipY };
                Vector2 v2 = { sx2 + perpX * headW, sy2 + perpY * headW };
                Vector2 v3 = { sx2 - perpX * headW, sy2 - perpY * headW };
                DrawTriangle(v1, v2, v3, game->projectile.color);
            } else {
                DrawRectangle((int)sx - 2, (int)sy - 2, 4, 4, game->projectile.color);
            }
        }
    }

    EndMode2D();

    // HUD (screen space)
    float scale = GetUIScale();
    int panelX = (int)(10 * scale);
    int panelW = (int)(260 * scale);
    int barW = panelW - (int)(20 * scale);
    int barH = (int)(16 * scale);
    int textY = 0;

    int playerLevel = 0, playerHp = 0, playerMaxHp = 0, playerExp = 0, playerExpToNext = 0;
    if (game->playerEntity != ENTITY_NONE) {
        const World* w = &game->ecs;
        const CStats* ps = World_GetStats((World*)w, game->playerEntity);
        playerLevel = ps->level;
        playerHp = ps->hp;
        playerMaxHp = ps->maxHp;
        playerExp = ps->exp;
        playerExpToNext = ps->expToNext;
    }

    char levelText[64];
    snprintf(levelText, sizeof(levelText), "Lv %d", playerLevel);
    int panelH = (int)(100 * scale);
    int panelY = GetScreenHeight() - (int)(10 * scale) - panelH;
    Texture2D* uiFrame = Resources_LoadTexture("resources/sprites/ui/UI_Flat_Frame01a.png");
    if (uiFrame && uiFrame->id > 0)
        Draw9Slice(*uiFrame, (Rectangle){ (float)(panelX - 4), (float)panelY, (float)panelW, (float)panelH }, 16, 16, 16, 16);
    else
        DrawRectangle(panelX - 4, panelY, panelW, panelH, (Color){ 0, 0, 0, 180 });
    textY = panelY + (int)(4 * scale);
    DrawText(levelText, panelX, textY, (int)(18 * scale), BLACK);

    // HP bar
    textY += (int)(24 * scale);
    float hpRatio = (playerMaxHp > 0) ? (float)playerHp / (float)playerMaxHp : 0.0f;
    if (hpRatio < 0) hpRatio = 0;
    DrawRectangle(panelX, textY, barW, barH, (Color){ 60, 0, 0, 255 });
    DrawRectangle(panelX, textY, (int)(barW * hpRatio), barH, RED);
    char hpText[64];
    snprintf(hpText, sizeof(hpText), "HP: %d/%d", playerHp, playerMaxHp);
    DrawText(hpText, panelX + (int)(4 * scale), textY + (int)(1 * scale), (int)(14 * scale), WHITE);

    // EXP bar
    textY += barH + (int)(6 * scale);
    float expRatio = (playerExpToNext > 0) ? (float)playerExp / (float)playerExpToNext : 0.0f;
    if (expRatio < 0) expRatio = 0;
    DrawRectangle(panelX, textY, barW, barH, (Color){ 0, 0, 60, 255 });
    DrawRectangle(panelX, textY, (int)(barW * expRatio), barH, (Color){ 80, 80, 255, 255 });
    char expText[64];
    snprintf(expText, sizeof(expText), "EXP: %d/%d", playerExp, playerExpToNext);
    DrawText(expText, panelX + (int)(4 * scale), textY + (int)(1 * scale), (int)(14 * scale), WHITE);

    // Floor info
    textY += barH + (int)(6 * scale);
    char infoText[64];
    snprintf(infoText, sizeof(infoText), "Floor: %d/%d", game->currentFloor, game->maxFloors);
    DrawText(infoText, panelX, textY, (int)(14 * scale), BLACK);

    // Combat log (bottom-right)
    {
        Texture2D* logFrame = Resources_LoadTexture("resources/sprites/ui/UI_Flat_Frame01a.png");
        Texture2D* logSlot = Resources_LoadTexture("resources/sprites/ui/UI_Flat_FrameSlot01b.png");
        CombatLog_Render(&game->combatLog, GetScreenWidth() - (int)(370 * scale), GetScreenHeight() - (int)(10 * scale), 14, (int)(18 * scale),
                         logFrame && logFrame->id > 0 ? *logFrame : (Texture2D){0}, 16,
                         logSlot && logSlot->id > 0 ? *logSlot : (Texture2D){0}, 8);
    }

    // Monster info (top-right)
    float iscale = GetUIScale();
    Inspector_Render(game, INSPECTOR_MONSTER, GetScreenWidth() - (int)(200 * iscale), (int)(10 * iscale), (int)(190 * iscale), (int)(160 * iscale));

    // Potion / equipment tile info (top-right, below monster info)
    if (game->selectedPotionTileActive) {
        int ptx = game->selectedPotionTileX;
        int pty = game->selectedPotionTileY;
        ItemType tileTypes[MAX_POTIONS];
        int tileQtys[MAX_POTIONS];
        int tileCount = SpawnerSystem_ListPotionsAt((GameWorld*)game, ptx, pty, tileTypes, tileQtys, MAX_POTIONS);
        EquipType equipInfos[MAX_EQUIP_ON_MAP];
        int equipQtys[MAX_EQUIP_ON_MAP];
        int equipCount = SpawnerSystem_ListEquipAt((GameWorld*)game, ptx, pty, equipInfos, equipQtys, MAX_EQUIP_ON_MAP);

        if (tileCount > 0 || equipCount > 0) {
            int totalItems = tileCount + equipCount;
            int pX = GetScreenWidth() - (int)(200 * scale);
            int pY = (int)(180 * scale);
            int pW = (int)(190 * scale);
            int fs = (int)(14 * scale);
            int lh = fs + (int)(4 * scale);
            int itemH = (int)(2 * lh) + (int)(4 * scale);
            int pH = (int)(40 * scale) + totalItems * itemH;
            if (pH > GetScreenHeight() - pY - (int)(10 * scale)) pH = GetScreenHeight() - pY - (int)(10 * scale);
            int ly = pY + (int)(6 * scale);
            char buf[128];

            Texture2D* slotTex = Resources_LoadTexture("resources/sprites/ui/UI_Flat_FrameSlot01b.png");
            if (slotTex && slotTex->id > 0)
                Draw9Slice(*slotTex, (Rectangle){ (float)pX, (float)pY, (float)pW, (float)pH }, 8, 8, 8, 8);
            else {
                DrawRectangle(pX, pY, pW, pH, (Color){ 0, 0, 0, 200 });
                DrawRectangleLines(pX, pY, pW, pH, (Color){ 80, 80, 80, 255 });
            }

            for (int i = 0; i < tileCount; i++) {
                const char* name = GetItemName(tileTypes[i]);
                int heal = GetItemHealAmount(tileTypes[i]);
                int qty = tileQtys[i];

                snprintf(buf, sizeof(buf), "%s x%d", name, qty);
                DrawText(buf, pX + (int)(6 * scale), ly, fs, BLACK); ly += lh;

                snprintf(buf, sizeof(buf), "Heals ~%d%% HP", heal);
                DrawText(buf, pX + (int)(6 * scale), ly, fs, BLACK); ly += lh;

                ly += (int)(4 * scale);
            }
            for (int i = 0; i < equipCount; i++) {
                const EquipData* d = GetEquipData(equipInfos[i]);
                if (!d) continue;
                int qty = equipQtys[i];
                snprintf(buf, sizeof(buf), "%s%s", d->name, qty > 1 ? " x2" : "");
                DrawText(buf, pX + (int)(6 * scale), ly, fs, BLACK); ly += lh;

                snprintf(buf, sizeof(buf), "%s", d->description);
                DrawText(buf, pX + (int)(6 * scale), ly, fs, BLACK); ly += lh;

                ly += (int)(4 * scale);
            }
        }
    }

    // State text (bottom center)
    const char* stateText = "";
    if (game->state == STATE_GAME_OVER) stateText = "GAME OVER - Hold Shift+R to restart";
    else if (game->state == STATE_WIN) stateText = "YOU WIN! - Hold Shift+R to restart";
    else if (game->state == STATE_PLAYER_TURN) stateText = "Your turn";
    else if (game->state == STATE_ENEMY_TURN) stateText = "Enemy turn...";

    if (stateText[0]) {
        int textWidth = MeasureText(stateText, (int)(20 * scale));
        DrawText(stateText, (GetScreenWidth() - textWidth) / 2, GetScreenHeight() - (int)(40 * scale), (int)(20 * scale), YELLOW);
    }

    // Level-up notification overlay
    if (game->levelUpTimer > 0.0f) {
        float alpha = game->levelUpTimer / 3.0f;
        float textScale = 1.0f + (1.0f - alpha) * 0.3f;
        int fontSize = (int)(48 * scale * textScale);
        int subSize = (int)(24 * scale * textScale);
        const char* title = "LEVEL UP!";
        const char* sub = "+2 Stat Points!";
        int tw = MeasureText(title, fontSize);
        int sw = MeasureText(sub, subSize);
        int cx = GetScreenWidth() / 2;
        int cy = GetScreenHeight() / 2;

        int glowW = (int)(fmaxf(tw, sw) * 1.5f);
        int glowH = (int)((fontSize + subSize + 40) * 1.5f);
        Color bg = { 0, 0, 0, (unsigned char)(180 * alpha) };
        DrawRectangle(cx - glowW / 2, cy - glowH / 2, glowW, glowH, bg);
        DrawRectangleLines(cx - glowW / 2, cy - glowH / 2, glowW, glowH, (Color){ 255, 255, 0, (unsigned char)(200 * alpha) });

        Color titleColor = { 255, 215, 0, (unsigned char)(255 * alpha) };
        DrawText(title, cx - tw / 2, cy - fontSize - (int)(8 * scale), fontSize, titleColor);

        Color subColor = { 255, 255, 255, (unsigned char)(255 * alpha) };
        DrawText(sub, cx - sw / 2, cy + (int)(8 * scale), subSize, subColor);
    }

    // Inventory overlay
    InventoryUI_Render((GameWorld*)game, ui);
}
