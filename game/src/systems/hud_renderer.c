#include "systems/hud_renderer.h"
#include "game.h"
#include "systems/renderer.h"
#include "resources.h"
#include "inventory.h"
#include <stdio.h>

void HUDRenderer_Render(GameWorld* game) {
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

    textY += (int)(24 * scale);
    float hpRatio = (playerMaxHp > 0) ? (float)playerHp / (float)playerMaxHp : 0.0f;
    if (hpRatio < 0) hpRatio = 0;
    DrawRectangle(panelX, textY, barW, barH, (Color){ 60, 0, 0, 255 });
    DrawRectangle(panelX, textY, (int)(barW * hpRatio), barH, RED);
    char hpText[64];
    snprintf(hpText, sizeof(hpText), "HP: %d/%d", playerHp, playerMaxHp);
    DrawText(hpText, panelX + (int)(4 * scale), textY + (int)(1 * scale), (int)(14 * scale), WHITE);

    textY += barH + (int)(6 * scale);
    float expRatio = (playerExpToNext > 0) ? (float)playerExp / (float)playerExpToNext : 0.0f;
    if (expRatio < 0) expRatio = 0;
    DrawRectangle(panelX, textY, barW, barH, (Color){ 0, 0, 60, 255 });
    DrawRectangle(panelX, textY, (int)(barW * expRatio), barH, (Color){ 80, 80, 255, 255 });
    char expText[64];
    snprintf(expText, sizeof(expText), "EXP: %d/%d", playerExp, playerExpToNext);
    DrawText(expText, panelX + (int)(4 * scale), textY + (int)(1 * scale), (int)(14 * scale), WHITE);

    textY += barH + (int)(6 * scale);
    char infoText[64];
    snprintf(infoText, sizeof(infoText), "Floor: %d/%d", game->currentFloor, game->maxFloors);
    DrawText(infoText, panelX, textY, (int)(14 * scale), BLACK);

    if (game->state != STATE_GAME_OVER && game->state != STATE_WIN) {
        char goldText[32];
        snprintf(goldText, sizeof(goldText), "Gold: %dg", game->gold);
        int goldW = MeasureText(goldText, (int)(14 * scale));
        DrawText(goldText, GetScreenWidth() - goldW - (int)(10 * scale), (int)(10 * scale), (int)(14 * scale), GOLD);
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

    // Combat action hints
    if (game->state == STATE_PLAYER_TURN && game->playerEntity != ENTITY_NONE) {
        EquipType weapon = game->equipped[EQUIP_SLOT_WEAPON];
        const EquipData* wdata = GetEquipData(weapon);
        int hintX = panelX + panelW + (int)(10 * scale);
        int hintY = panelY + (int)(4 * scale);
        int hintFs = (int)(12 * scale);
        if (hintFs < 8) hintFs = 8;
        DrawText("[F] Attack", hintX, hintY, hintFs, DARKGRAY);
        if (wdata && !wdata->isRanged && weapon != EQUIP_NONE) {
            DrawText("[T] Throw Weapon", hintX, hintY + (int)(14 * scale), hintFs, DARKGRAY);
        }
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
}
