#include "game.h"
#include "entity/monster.h"
#include "ui/combat_log.h"
#include "ui/monster_info.h"
#include <stdio.h>
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
            int cellStride = 17;
            Rectangle src = {
                (float)(game->player.ent.animFrame * cellStride),
                (float)(game->player.ent.spriteRow * cellStride),
                16.0f, 16.0f
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

            if (game->player.ent.facingRight) {
                DrawCircle(cx + 5, cy + 4, 1, WHITE);
            } else {
                DrawCircle(cx - 5, cy + 4, 1, WHITE);
            }
        }
    }

    EndMode2D();

    // HUD (screen space)
    int panelX = 10;
    int panelW = 260;
    int barW = panelW - 20;
    int barH = 16;
    int textY = 0;

    char levelText[64];
    snprintf(levelText, sizeof(levelText), "Lv %d", game->player.ent.level);
    int panelH = 100;
    int panelY = GetScreenHeight() - 10 - panelH;
    if (game->texUiFrame.id > 0)
        Draw9Slice(game->texUiFrame, (Rectangle){ (float)(panelX - 4), (float)panelY, (float)panelW, (float)panelH }, 16, 16, 16, 16);
    else
        DrawRectangle(panelX - 4, panelY, panelW, panelH, (Color){ 0, 0, 0, 180 });
    textY = panelY + 4;
    DrawText(levelText, panelX, textY, 18, BLACK);

    // HP bar
    textY += 24;
    float hpRatio = (float)game->player.ent.hp / (float)game->player.ent.maxHp;
    if (hpRatio < 0) hpRatio = 0;
    DrawRectangle(panelX, textY, barW, barH, (Color){ 60, 0, 0, 255 });
    DrawRectangle(panelX, textY, (int)(barW * hpRatio), barH, RED);
    char hpText[64];
    snprintf(hpText, sizeof(hpText), "HP: %d/%d", game->player.ent.hp, game->player.ent.maxHp);
    DrawText(hpText, panelX + 4, textY + 1, 14, WHITE);

    // EXP bar
    textY += barH + 6;
    float expRatio = (float)game->player.exp / (float)game->player.expToNext;
    if (expRatio < 0) expRatio = 0;
    DrawRectangle(panelX, textY, barW, barH, (Color){ 0, 0, 60, 255 });
    DrawRectangle(panelX, textY, (int)(barW * expRatio), barH, (Color){ 80, 80, 255, 255 });
    char expText[64];
    snprintf(expText, sizeof(expText), "EXP: %d/%d", game->player.exp, game->player.expToNext);
    DrawText(expText, panelX + 4, textY + 1, 14, WHITE);

    // Floor info
    textY += barH + 6;
    char infoText[64];
    snprintf(infoText, sizeof(infoText), "Floor: %d/%d", game->currentFloor, game->maxFloors);
    DrawText(infoText, panelX, textY, 14, BLACK);

    // Combat log (bottom-right)
    CombatLog_Render(&game->combatLog, GetScreenWidth() - 370, GetScreenHeight() - 10, 14, 18, game->texUiFrame, 16);

    MonsterInfo_Render(game);

    // Potion tile info (top-right, below monster info)
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

        if (tileCount > 0) {
            int pX = GetScreenWidth() - 200;
            int pY = 180;
            int pW = 190;
            int pH = 40 + tileCount * 50;
            if (pH > GetScreenHeight() - pY - 10) pH = GetScreenHeight() - pY - 10;
            int ly = pY + 6;
            int fs = 14;
            int lh = fs + 4;
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
                DrawText(buf, pX + 6, ly, fs, YELLOW); ly += lh;

                snprintf(buf, sizeof(buf), "Heals %d HP", heal);
                DrawText(buf, pX + 6, ly, fs, (Color){ 0, 90, 0, 255 }); ly += lh;

                ly += 4;
            }
        }
    }

    // State text (bottom center)
    const char* stateText = "";
    if (game->state == STATE_GAME_OVER) stateText = "GAME OVER - Press R to restart";
    else if (game->state == STATE_WIN) stateText = "YOU WIN! - Press R to restart";
    else if (game->state == STATE_PLAYER_TURN) stateText = "Your turn";
    else if (game->state == STATE_ENEMY_TURN) stateText = "Enemy turn...";

    if (stateText[0]) {
        int textWidth = MeasureText(stateText, 20);
        DrawText(stateText, (GetScreenWidth() - textWidth) / 2, GetScreenHeight() - 40, 20, YELLOW);
    }

    // Inventory overlay
    Inventory_Render(game);
}
