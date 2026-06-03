#include "map_ui.h"
#include "map/procedural.h"
#include "raylib.h"
#include <stdio.h>

void MapUI_Update(GameWorld* gw) {
    if (!gw) return;
    if (IsKeyPressed(KEY_ESCAPE) ||
        IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) ||
        IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) ||
        IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) ||
        IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        gw->state = STATE_PLAYER_TURN;
    }
}

void MapUI_Render(const GameWorld* gw) {
    if (!gw || !gw->map) return;
    if (gw->state != STATE_MAP) return;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int mapW = gw->map->width;
    int mapH = gw->map->height;

    float availW = (float)sw * 0.8f;
    float availH = (float)sh * 0.8f;
    int tileSize = (int)(availW / mapW);
    int tileH = (int)(availH / mapH);
    if (tileH < tileSize) tileSize = tileH;
    if (tileSize < 2) tileSize = 2;

    int mapPixelW = mapW * tileSize;
    int mapPixelH = mapH * tileSize;
    int ox = (sw - mapPixelW) / 2;
    int oy = (sh - mapPixelH) / 2;

    DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 200 });

    char title[64];
    snprintf(title, sizeof(title), "MAP  -  Floor %d", gw->currentFloor);
    int fs = (int)(20 * (float)sw / 1024.0f);
    if (fs < 12) fs = 12;
    int tw = MeasureText(title, fs);
    DrawText(title, (sw - tw) / 2, oy - fs - 10, fs, WHITE);

    for (int y = 0; y < mapH; y++) {
        for (int x = 0; x < mapW; x++) {
            int px = ox + x * tileSize;
            int py = oy + y * tileSize;
            unsigned char vis = gw->visibility[y][x];

            if (vis == 0) {
                DrawRectangle(px, py, tileSize, tileSize, (Color){ 0, 0, 0, 255 });
            } else {
                if (gw->blocking[y][x]) {
                    DrawRectangle(px, py, tileSize, tileSize, (Color){ 60, 60, 80, 255 });
                } else {
                    DrawRectangle(px, py, tileSize, tileSize, (Color){ 140, 120, 100, 255 });
                }
            }
        }
    }

    if (gw->playerEntity != ENTITY_NONE) {
        const CPosition* pp = World_GetPosition((World*)&gw->ecs, gw->playerEntity);
        if (pp && gw->visibility[pp->y][pp->x] > 0) {
            int px = ox + pp->x * tileSize + tileSize / 2;
            int py = oy + pp->y * tileSize + tileSize / 2;
            int r = (tileSize / 3 > 1) ? tileSize / 3 : 1;
            DrawCircle(px, py, (float)r, (Color){ 0, 255, 255, 255 });
        }
    }

    if (gw->stairX >= 0 && gw->stairY >= 0 && gw->visibility[gw->stairY][gw->stairX] > 0) {
        int px = ox + gw->stairX * tileSize;
        int py = oy + gw->stairY * tileSize;
        DrawRectangle(px, py, tileSize, tileSize, (Color){ 255, 255, 0, 200 });
        if (tileSize >= 6)
            DrawText(">", px + 1, py, tileSize - 1, (Color){ 0, 0, 0, 255 });
    }

    if (gw->escapeSpawned && gw->escapeX >= 0 && gw->escapeY >= 0 &&
        gw->visibility[gw->escapeY][gw->escapeX] > 0) {
        int px = ox + gw->escapeX * tileSize;
        int py = oy + gw->escapeY * tileSize;
        DrawRectangle(px, py, tileSize, tileSize, (Color){ 0, 200, 0, 200 });
        if (tileSize >= 6)
            DrawText("E", px + 1, py, tileSize - 1, (Color){ 0, 0, 0, 255 });
    }

    // Pickup markers (orange)
    for (EntityId e = 1; e < (EntityId)gw->ecs.count; e++) {
        if (!gw->ecs.alive[e]) continue;
        if (!World_HasComponents((World*)&gw->ecs, e, COMP_POSITION | COMP_PICKUP)) continue;
        const CPickup* pk = World_GetPickup((World*)&gw->ecs, e);
        if (pk->quantity <= 0) continue;
        const CPosition* p = World_GetPosition((World*)&gw->ecs, e);
        if (p->y < 0 || p->y >= mapH || p->x < 0 || p->x >= mapW) continue;
        if (gw->visibility[p->y][p->x] == 0) continue;
        int px = ox + p->x * tileSize;
        int py = oy + p->y * tileSize;
        DrawRectangle(px, py, tileSize, tileSize, (Color){ 255, 140, 0, 200 });
    }

    int legendY = oy + mapPixelH + 8;
    int legendFs = (int)(14 * (float)sw / 1024.0f);
    if (legendFs < 10) legendFs = 10;
    int lx = ox;

    DrawCircle((float)(lx + 6), (float)(legendY + legendFs / 2), 4, (Color){ 0, 255, 255, 255 });
    DrawText("Player", lx + 14, legendY, legendFs, WHITE);
    int playerW = MeasureText("Player", legendFs);

    DrawRectangle(lx + 14 + playerW + 10, legendY, legendFs, legendFs, (Color){ 255, 255, 0, 200 });
    DrawText("Stairs", lx + 14 + playerW + 10 + legendFs + 4, legendY, legendFs, WHITE);
    int stairsW = MeasureText("Stairs", legendFs);

    DrawRectangle(lx + 14 + playerW + 10 + legendFs + 4 + stairsW + 10, legendY, legendFs, legendFs, (Color){ 0, 200, 0, 200 });
    DrawText("Escape", lx + 14 + playerW + 10 + legendFs + 4 + stairsW + 10 + legendFs + 4, legendY, legendFs, WHITE);
    int escapeW = MeasureText("Escape", legendFs);
    int orangeX = lx + 14 + playerW + 10 + legendFs + 4 + stairsW + 10 + legendFs + 4 + escapeW + 10;
    DrawRectangle(orangeX, legendY, legendFs, legendFs, (Color){ 255, 140, 0, 200 });
    DrawText("Items", orangeX + legendFs + 4, legendY, legendFs, WHITE);
}
