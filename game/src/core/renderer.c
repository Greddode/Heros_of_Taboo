#include "game.h"
#include "entity/monster.h"
#include "ui/combat_log.h"
#include "ui/inspector.h"
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

void RenderGame(const Game* game) {
    if (!game || !game->map) return;

    int tw = game->map->tileWidth;
    int th = game->map->tileHeight;

    BeginMode2D(game->camera);

    for (int layer = 0; layer < game->map->layerCount; layer++) {
        if (!game->map->layers[layer].visible) continue;

        for (int y = 0; y < game->map->height; y++) {
            for (int x = 0; x < game->map->width; x++) {
                unsigned char vis = game->visibility[y][x];
                if (vis == 0) continue;

                DrawTile(game, x, y, layer);

                if (vis == 2) {
                    Vector2 pos = TileToScreen(x, y, tw, th);
                    DrawRectangle(pos.x, pos.y, tw, th, (Color){ 0, 0, 0, 180 });
                }
            }
        }
    }

    // Potion sprites on the map
    for (int i = 0; i < game->potionCount; i++) {
        if (game->potionCollected[i]) continue;
        int hx = game->potionTiles[i][0];
        int hy = game->potionTiles[i][1];
        if (game->visibility[hy][hx] != 1) continue;

        int texIdx = game->potionTypes[i] - 1;
        if (texIdx >= 0 && texIdx < 3 && game->potionTextures[texIdx].id > 0) {
            Vector2 hpos = TileToScreen(hx, hy, tw, th);
            Rectangle dest = { hpos.x, hpos.y, (float)tw, (float)th };
            DrawTexturePro(game->potionTextures[texIdx],
                           (Rectangle){ 0, 0, (float)game->potionTextures[texIdx].width, (float)game->potionTextures[texIdx].height },
                           dest, (Vector2){ 0, 0 }, 0, WHITE);
            if (game->potionQuantities[i] > 1) {
                char qty[8];
                snprintf(qty, sizeof(qty), "x%d", game->potionQuantities[i]);
                DrawText(qty, hpos.x + 2, hpos.y + 2, 10, YELLOW);
            }
        }
    }

    // Equipment sprites on the map
    {
        // Track which tiles we've already rendered (avoids duplicates)
        bool renderedTiles[MAP_HEIGHT][MAP_WIDTH] = { false };
        for (int i = 0; i < game->equipMapCount; i++) {
            if (game->equipMapCollected[i]) continue;
            int ex = game->equipMapTiles[i][0];
            int ey = game->equipMapTiles[i][1];
            if (game->visibility[ey][ex] != 1) continue;
            if (renderedTiles[ey][ex]) continue;
            renderedTiles[ey][ex] = true;

            // Count how many equipment items are on this tile
            int itemCount = 0;
            for (int j = 0; j < game->equipMapCount; j++) {
                if (game->equipMapCollected[j]) continue;
                if (game->equipMapTiles[j][0] == ex && game->equipMapTiles[j][1] == ey)
                    itemCount++;
            }

            Vector2 epos = TileToScreen(ex, ey, tw, th);

            if (itemCount > 1) {
                // Multiple items on same tile -> show loot.png
                if (game->texLoot.id > 0) {
                    Rectangle dest = { epos.x, epos.y, (float)tw, (float)th };
                    DrawTexturePro(game->texLoot,
                                   (Rectangle){ 0, 0, (float)game->texLoot.width, (float)game->texLoot.height },
                                   dest, (Vector2){ 0, 0 }, 0, WHITE);
                }
                // Show item count on loot pile
                char qty[8];
                snprintf(qty, sizeof(qty), "x%d", itemCount);
                DrawText(qty, epos.x + 2, epos.y + 2, 10, YELLOW);
            } else {
                // Single type -> show its own sprite
                EquipType eType = game->equipMapTypes[i];
                Texture2D tex = GetEquipSprite(eType);
                if (tex.id > 0) {
                    Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
                    Rectangle dest = { epos.x, epos.y, (float)tw, (float)th };
                    DrawTexturePro(tex, src, dest, (Vector2){ 0 }, 0, WHITE);
                }
                // Show quantity label for stacked copies of same type
                int qty = game->equipMapQuantities[i];
                if (qty > 1) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "x%d", qty);
                    DrawText(buf, epos.x + 2, epos.y + 2, 10, YELLOW);
                }
            }
        }
    }

    // Interpolation factors
    float pd = (game->animDuration > 0.0f) ? game->animDuration : MOVE_ANIM_DURATION;
    float md = (game->monsterAnimDuration > 0.0f) ? game->monsterAnimDuration : MOVE_ANIM_DURATION;
    float playerT = (game->animTimer <= 0.0f) ? 1.0f : 1.0f - (game->animTimer / pd);
    float monsterT = (game->monsterAnimTimer <= 0.0f) ? 1.0f : 1.0f - (game->monsterAnimTimer / md);

    Monster_RenderAll(game->visibility, game->map->width, game->map->height, tw, th, monsterT);

    // Player rendering
    if (game->player.ent.alive) {
        float px = (float)(game->player.ent.prevX * tw) * (1.0f - playerT) + (float)(game->player.ent.x * tw) * playerT;
        float py = (float)(game->player.ent.prevY * th) * (1.0f - playerT) + (float)(game->player.ent.y * th) * playerT;

        if (game->player.ent.spriteSheet.id > 0) {
            int frameCount = 4;
            float frameW = (float)game->player.ent.spriteSheet.width / (float)frameCount;
            float frameH = (float)game->player.ent.spriteSheet.height;
            Rectangle src = {
                (float)(game->player.ent.animFrame * frameW),
                0,
                frameW, frameH
            };
            Rectangle dest = { px, py, (float)tw, (float)th };
            Color tint = (game->player.ent.hitFlashTimer > 0.0f) ? (Color){ 255, 255, 255, 200 } : WHITE;
            DrawTexturePro(game->player.ent.spriteSheet, src, dest, (Vector2){ 0, 0 }, 0, tint);
        } else {
            int pad = tw / 6;
            Color playerColor = (game->player.ent.hitFlashTimer > 0.0f) ? WHITE : game->player.ent.color;
            DrawRectangle(px + pad, py + pad, tw - 2*pad, th - 2*pad, playerColor);

            int cx = px + tw / 2;
            int cy = py + th / 2;
            DrawCircle(cx - 3, cy - 2, 2, WHITE);
            DrawCircle(cx + 3, cy - 2, 2, WHITE);

            if (game->player.ent.facingDir == DIR_RIGHT) {
                DrawCircle(cx + 5, cy + 4, 1, WHITE);
            } else if (game->player.ent.facingDir == DIR_LEFT) {
                DrawCircle(cx - 5, cy + 4, 1, WHITE);
            } else if (game->player.ent.facingDir == DIR_UP) {
                DrawCircle(cx, cy - 5, 1, WHITE);
            } else {
                DrawCircle(cx, cy + 5, 1, WHITE);
            }
        }
    }

    // Direction indicator: show a 32x32 dot on the adjacent tile in the player's facing direction
    if (game->player.ent.alive && game->state == STATE_PLAYER_TURN) {
        int dx = game->player.ent.x, dy = game->player.ent.y;
        switch (game->player.ent.facingDir) {
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

    // Projectile rendering (in world space)
    if (game->projectile.active && game->projectileDuration > 0.0f) {
        float pt = (game->projectileTimer <= 0.0f) ? 1.0f : 1.0f - (game->projectileTimer / game->projectileDuration);
        float cx = game->projectile.sx + (game->projectile.ex - game->projectile.sx) * pt;
        float cy = game->projectile.sy + (game->projectile.ey - game->projectile.sy) * pt;

        if (game->projectile.attackType == ATTACK_MAGIC &&
            game->magicAttacksTexture.id > 0 && game->projectile.animFrameCount > 0) {
            int x0 = game->projectile.tileSX, y0 = game->projectile.tileSY;
            int x1 = game->projectile.tileEX, y1 = game->projectile.tileEY;
            int tw = game->map->tileWidth, th = game->map->tileHeight;
            int totalDist = abs(x1 - x0) + abs(y1 - y0);
            int cols = 10, tileSize = 64;
            int animLen = game->projectile.animFrameCount;

            // Bresenham: iterate all tiles on the line
            int bdx = abs(x1 - x0), bdy = abs(y1 - y0);
            int sx = (x0 < x1) ? 1 : -1, sy = (y0 < y1) ? 1 : -1;
            int err = bdx - bdy;
            int x = x0, y = y0;
            while (1) {
                int tx = x, ty = y;

                // Wave effect: animation travels from caster toward target
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
                        Rectangle dest = { (float)(tx * tw), (float)(ty * th), (float)tw, (float)th };
                        DrawTexturePro(game->magicAttacksTexture, src, dest, (Vector2){ 0 }, 0, WHITE);
                    }
                }

                if (tx == x1 && ty == y1) break;
                int e2 = 2 * err;
                if (e2 > -bdy) { err -= bdy; x += sx; }
                if (e2 < bdx)  { err += bdx; y += sy; }
            }
        } else {
            // Pixel-art arrow for ranged attacks
            float sx = game->projectile.sx, sy = game->projectile.sy;
            float dx = cx - sx, dy = cy - sy;
            float len = sqrtf(dx * dx + dy * dy);
            if (len > 4.0f) {
                float nx = dx / len, ny = dy / len;
                // Shaft: thick line from source to near the head
                float headLen = 12.0f;
                float shaftEnd = len - headLen;
                if (shaftEnd < 0) shaftEnd = 0;
                float sx2 = sx + nx * shaftEnd, sy2 = sy + ny * shaftEnd;
                DrawLineEx((Vector2){ sx, sy }, (Vector2){ sx2, sy2 }, 3.0f, game->projectile.color);
                // Arrowhead: triangle at the tip
                float tipX = sx + nx * len, tipY = sy + ny * len;
                float px = -ny, py = nx; // perpendicular
                float headW = 6.0f;
                Vector2 v1 = { tipX, tipY };
                Vector2 v2 = { sx2 + px * headW, sy2 + py * headW };
                Vector2 v3 = { sx2 - px * headW, sy2 - py * headW };
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

    char levelText[64];
    snprintf(levelText, sizeof(levelText), "Lv %d", game->player.ent.level);
    int panelH = (int)(100 * scale);
    int panelY = GetScreenHeight() - (int)(10 * scale) - panelH;
    if (game->texUiFrame.id > 0)
        Draw9Slice(game->texUiFrame, (Rectangle){ (float)(panelX - 4), (float)panelY, (float)panelW, (float)panelH }, 16, 16, 16, 16);
    else
        DrawRectangle(panelX - 4, panelY, panelW, panelH, (Color){ 0, 0, 0, 180 });
    textY = panelY + (int)(4 * scale);
    DrawText(levelText, panelX, textY, (int)(18 * scale), BLACK);

    // HP bar
    textY += (int)(24 * scale);
    float hpRatio = (float)game->player.ent.hp / (float)game->player.ent.maxHp;
    if (hpRatio < 0) hpRatio = 0;
    DrawRectangle(panelX, textY, barW, barH, (Color){ 60, 0, 0, 255 });
    DrawRectangle(panelX, textY, (int)(barW * hpRatio), barH, RED);
    char hpText[64];
    snprintf(hpText, sizeof(hpText), "HP: %d/%d", game->player.ent.hp, game->player.ent.maxHp);
    DrawText(hpText, panelX + (int)(4 * scale), textY + (int)(1 * scale), (int)(14 * scale), WHITE);

    // EXP bar
    textY += barH + (int)(6 * scale);
    float expRatio = (float)game->player.exp / (float)game->player.expToNext;
    if (expRatio < 0) expRatio = 0;
    DrawRectangle(panelX, textY, barW, barH, (Color){ 0, 0, 60, 255 });
    DrawRectangle(panelX, textY, (int)(barW * expRatio), barH, (Color){ 80, 80, 255, 255 });
    char expText[64];
    snprintf(expText, sizeof(expText), "EXP: %d/%d", game->player.exp, game->player.expToNext);
    DrawText(expText, panelX + (int)(4 * scale), textY + (int)(1 * scale), (int)(14 * scale), WHITE);

    // Floor info
    textY += barH + (int)(6 * scale);
    char infoText[64];
    snprintf(infoText, sizeof(infoText), "Floor: %d/%d", game->currentFloor, game->maxFloors);
    DrawText(infoText, panelX, textY, (int)(14 * scale), BLACK);

    // Combat log (bottom-right)
    CombatLog_Render(&game->combatLog, GetScreenWidth() - (int)(370 * scale), GetScreenHeight() - (int)(10 * scale), 14, (int)(18 * scale), game->texUiFrame, 16, game->texUiSlot, 8);

    // Monster info (top-right)
    float iscale = GetUIScale();
    Inspector_Render(game, INSPECTOR_MONSTER, GetScreenWidth() - (int)(200 * iscale), (int)(10 * iscale), (int)(190 * iscale), (int)(160 * iscale));

    // Potion / equipment tile info (top-right, below monster info)
    if (game->selectedPotionTileActive) {
        int ptx = game->selectedPotionTileX;
        int pty = game->selectedPotionTileY;
        ItemType tileTypes[MAX_POTIONS];
        int tileQtys[MAX_POTIONS];
        int tileCount = 0;
        for (int p = 0; p < game->potionCount; p++) {
            if (game->potionCollected[p]) continue;
            if (game->potionTiles[p][0] == ptx && game->potionTiles[p][1] == pty) {
                tileTypes[tileCount] = game->potionTypes[p];
                tileQtys[tileCount] = game->potionQuantities[p];
                tileCount++;
                if (tileCount >= MAX_POTIONS) break;
            }
        }
        // Also collect equipment at this tile
        EquipType equipInfos[MAX_EQUIP_ON_MAP];
        int equipQtys[MAX_EQUIP_ON_MAP];
        int equipCount = 0;
        for (int e = 0; e < game->equipMapCount; e++) {
            if (game->equipMapCollected[e]) continue;
            if (game->equipMapTiles[e][0] == ptx && game->equipMapTiles[e][1] == pty) {
                equipInfos[equipCount] = game->equipMapTypes[e];
                equipQtys[equipCount] = game->equipMapQuantities[e];
                equipCount++;
                if (equipCount >= MAX_EQUIP_ON_MAP) break;
            }
        }

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

            if (game->texUiSlot.id > 0)
                Draw9Slice(game->texUiSlot, (Rectangle){ (float)pX, (float)pY, (float)pW, (float)pH }, 8, 8, 8, 8);
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

    // Level-up notification overlay (screen center, fades out)
    if (game->levelUpTimer > 0.0f) {
        float alpha = game->levelUpTimer / 3.0f; // 1.0 -> 0.0 over 3 seconds
        float textScale = 1.0f + (1.0f - alpha) * 0.3f; // shrink from 1.3x to 1.0x
        int fontSize = (int)(48 * scale * textScale);
        int subSize = (int)(24 * scale * textScale);
        const char* title = "LEVEL UP!";
        const char* sub = "+2 Stat Points!";
        int tw = MeasureText(title, fontSize);
        int sw = MeasureText(sub, subSize);
        int cx = GetScreenWidth() / 2;
        int cy = GetScreenHeight() / 2;

        // Background glow
        int glowW = (int)(fmaxf(tw, sw) * 1.5f);
        int glowH = (int)((fontSize + subSize + 40) * 1.5f);
        Color bg = { 0, 0, 0, (unsigned char)(180 * alpha) };
        DrawRectangle(cx - glowW / 2, cy - glowH / 2, glowW, glowH, bg);
        DrawRectangleLines(cx - glowW / 2, cy - glowH / 2, glowW, glowH, (Color){ 255, 255, 0, (unsigned char)(200 * alpha) });

        // Title text (gold with fade)
        Color titleColor = { 255, 215, 0, (unsigned char)(255 * alpha) };
        DrawText(title, cx - tw / 2, cy - fontSize - (int)(8 * scale), fontSize, titleColor);

        // Subtitle text (white with fade)
        Color subColor = { 255, 255, 255, (unsigned char)(255 * alpha) };
        DrawText(sub, cx - sw / 2, cy + (int)(8 * scale), subSize, subColor);
    }

    // Inventory overlay
    Inventory_Render(game);
}
