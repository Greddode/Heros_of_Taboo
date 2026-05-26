#include "game.h"
#include "entity/entity.h"
#include "entity/player.h"
#include "ui/combat_log.h"
#include "ui/monster_info.h"
#include "entity/monster.h"
#include "procedural.h"
#include "entity/spawner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Check if a tile lies within any generated room's bounds.
static bool IsInRoom(int x, int y) {
    ProceduralRoom rooms[MAX_GENERATED_ROOMS];
    int count = GetGeneratedRooms(rooms, MAX_GENERATED_ROOMS);
    for (int i = 0; i < count; i++) {
        if (x >= rooms[i].x && x < rooms[i].x + rooms[i].w &&
            y >= rooms[i].y && y < rooms[i].y + rooms[i].h)
            return true;
    }
    return false;
}

// Simple circular radius reveal: all tiles within FOG_RADIUS are lit.
// Previously-seen tiles are dimmed.
static void RevealFOW(Game* game) {
    int px = game->player.ent.x;
    int py = game->player.ent.y;

    for (int y = 0; y < game->map->height; y++) {
        for (int x = 0; x < game->map->width; x++) {
            if (game->visibility[y][x] == 1)
                game->visibility[y][x] = 2;
        }
    }

    int r2 = FOG_RADIUS * FOG_RADIUS;
    for (int dy = -FOG_RADIUS; dy <= FOG_RADIUS; dy++) {
        int ny = py + dy;
        if (ny < 0 || ny >= game->map->height) continue;
        for (int dx = -FOG_RADIUS; dx <= FOG_RADIUS; dx++) {
            int nx = px + dx;
            if (nx < 0 || nx >= game->map->width) continue;
            if (dx * dx + dy * dy <= r2)
                game->visibility[ny][nx] = 1;
        }
    }
}

// Place the escape teleport tile on a random floor tile when all monsters on floor 10 are dead.
static void SpawnEscapeTile(Game* game) {
    int w = game->map->width;
    int h = game->map->height;
    int* data = game->map->layers[0].data;

    for (int attempt = 0; attempt < 200; attempt++) {
        int tx = GetRandomValue(3, w - 4);
        int ty = GetRandomValue(3, h - 4);
        if (IsFloorGID(data[ty * w + tx])) {
            data[ty * w + tx] = TILE_ESCAPE;
            game->escapeX = tx;
            game->escapeY = ty;
            TraceLog(LOG_INFO, "Escape tile placed at (%d,%d)", tx, ty);
            return;
        }
    }
    TraceLog(LOG_WARNING, "Escape tile: could not find valid position");
}

// Locate a walkable tile away from the player and spawn the shadow monster,
// scaled to twice the player's level.
static void SpawnShadow(Game* game) {
    int px = game->player.ent.x;
    int py = game->player.ent.y;
    int w = game->map->width;
    int h = game->map->height;

    for (int attempt = 0; attempt < 100; attempt++) {
        int tx = GetRandomValue(3, w - 4);
        int ty = GetRandomValue(3, h - 4);
        int dx = tx - px;
        int dy = ty - py;
        if (dx * dx + dy * dy >= 25 && !game->blocking[ty][tx] && IsFloorGID(GetTileGID(game->map, 0, tx, ty))) {
            Monster* shadow = Monster_Spawn(MONSTER_SHADOW, tx, ty, 1);
            if (shadow) {
                int targetLevel = game->player.ent.level * 2;
                if (targetLevel < 10) targetLevel = 10;
                int extra = targetLevel - 1;
                if (extra > 0) {
                    shadow->level    = targetLevel;
                    shadow->maxHp   += extra * 2;
                    shadow->hp       = shadow->maxHp;
                    shadow->attack  += extra;
                    shadow->defense += extra / 2;
                    shadow->expValue += extra * 5;
                }
                shadow->shadowTurnCounter = 0;
                TraceLog(LOG_INFO, "Shadow spawned at (%d,%d) level %d", tx, ty, targetLevel);
            }
            return;
        }
    }
    TraceLog(LOG_WARNING, "Shadow: could not find valid spawn position");
}

// Read directional keys (arrow/WASD) and pass them to MoveEntity.
// Period/space waits a turn, restoring 1 HP.
// After any action, control passes to the enemy turn.
void HandleInput(Game* game) {
    if (game->state == STATE_INVENTORY) {
        if (IsKeyPressed(KEY_I) || IsKeyPressed(KEY_ESCAPE)) {
            game->state = STATE_PLAYER_TURN;
        }
        if (game->inventorySlotCount > 0) {
            if (IsKeyPressed(KEY_UP) && game->inventorySelection > 0)
                game->inventorySelection--;
            if (IsKeyPressed(KEY_DOWN) && game->inventorySelection < game->inventorySlotCount - 1)
                game->inventorySelection++;
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
                InventoryUse(game, game->inventorySelection);
        }
        return;
    }

    if (game->state != STATE_PLAYER_TURN) return;
    if (game->animTimer > 0.0f) return;

    // Mouse click: select monster at cursor position
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        game->sprintBypassRoom = false;
        Vector2 worldPos = GetScreenToWorld2D(GetMousePosition(), game->camera);
        int tileX = (int)(worldPos.x / game->map->tileWidth);
        int tileY = (int)(worldPos.y / game->map->tileHeight);
        if (tileX >= 0 && tileX < game->map->width &&
            tileY >= 0 && tileY < game->map->height &&
            game->visibility[tileY][tileX] == 1) {
            Monster* mon = Monster_GetAt(tileX, tileY, NULL);
            if (mon) {
                Monster* arr = Monster_GetArray();
                int count = Monster_GetCount();
                game->selectedMonsterIdx = -1;
                for (int i = 0; i < count; i++) {
                    if (&arr[i] == mon) {
                        game->selectedMonsterIdx = i;
                        break;
                    }
                }
        }
        } else {
            game->selectedMonsterIdx = -1;
        }
    }

    // Sprint: hold SHIFT + direction to slide to nearest obstacle
    if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
        Direction sprintDir = DIR_NONE;
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) sprintDir = DIR_UP;
        else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) sprintDir = DIR_DOWN;
        else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) sprintDir = DIR_LEFT;
        else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) sprintDir = DIR_RIGHT;

        if (sprintDir != DIR_NONE) {
            bool stoppedAtRoom = false;
            int dx = 0, dy = 0;
            switch (sprintDir) {
                case DIR_UP:    dy = -1; break;
                case DIR_DOWN:  dy = 1;  break;
                case DIR_LEFT:  dx = -1; break;
                case DIR_RIGHT: dx = 1;  break;
                default: break;
            }

            int startX = game->player.ent.x;
            int startY = game->player.ent.y;
            int endX = startX, endY = startY;

            while (1) {
                int nx = endX + dx;
                int ny = endY + dy;
                if (nx < 0 || nx >= game->map->width || ny < 0 || ny >= game->map->height) break;
                if (game->blocking[ny][nx]) break;
                if (Monster_GetAt(nx, ny, NULL)) break;
                endX = nx;
                endY = ny;
            }

            int steps = abs(endX - startX) + abs(endY - startY);
            if (steps > 0) {
                // Snapshot monster positions so sprite interpolation works smoothly
                Monster* monArray = Monster_GetArray();
                int monCount = Monster_GetCount();
                typedef struct { int x, y; float hitFlash; } MonSnap;
                MonSnap monSnap[64];
                int snapCount = monCount < 64 ? monCount : 64;
                for (int i = 0; i < snapCount; i++) {
                    monSnap[i].x = monArray[i].x;
                    monSnap[i].y = monArray[i].y;
                    monSnap[i].hitFlash = monArray[i].hitFlashTimer;
                }

                game->player.ent.prevX = startX;
                game->player.ent.prevY = startY;
                game->player.ent.x = startX;
                game->player.ent.y = startY;
                if (sprintDir == DIR_RIGHT) game->player.ent.facingRight = true;
                else if (sprintDir == DIR_LEFT) game->player.ent.facingRight = false;
                game->selectedMonsterIdx = -1;

                for (int s = 0; s < steps; s++) {
                    int nx = game->player.ent.x + dx;
                    int ny = game->player.ent.y + dy;

                    // Re-check obstacles (a monster may have moved into the path)
                    if (nx < 0 || nx >= game->map->width || ny < 0 || ny >= game->map->height) break;
                    if (game->blocking[ny][nx]) break;
                    if (Monster_GetAt(nx, ny, NULL)) break;

                    // Stop when crossing between room and hallway; sprint again to bypass
                    if (IsInRoom(game->player.ent.x, game->player.ent.y) != IsInRoom(nx, ny)) {
                        if (game->sprintBypassRoom) {
                            game->sprintBypassRoom = false;
                        } else {
                            game->sprintBypassRoom = true;
                            stoppedAtRoom = true;
                            break;
                        }
                    }

                    game->player.ent.x = nx;
                    game->player.ent.y = ny;
                    game->turnCount++;

                    bool alive = Monster_ProcessAllAI(
                        game->player.ent.x, game->player.ent.y,
                        &game->player.ent.hp, game->player.ent.defense,
                        &game->player.ent.hitFlashTimer,
                        game->blocking, game->map->width, game->map->height,
                        &game->combatLog, game->timeWaited);

                    if (!alive) {
                        game->state = STATE_GAME_OVER;
                        return;
                    }
                }

                if (!stoppedAtRoom) game->sprintBypassRoom = false;

                // Restore original positions as prev for smooth interpolation
                for (int i = 0; i < snapCount; i++) {
                    monArray[i].prevX = monSnap[i].x;
                    monArray[i].prevY = monSnap[i].y;
                    monArray[i].hitFlashTimer = monSnap[i].hitFlash;
                }

                game->animTimer = 0.30f;
                game->animDuration = 0.30f;
                game->monsterAnimTimer = 0.30f;
                game->monsterAnimDuration = 0.30f;
                RevealFOW(game);

                if (game->player.ent.x == game->stairX && game->player.ent.y == game->stairY &&
                    game->stairX >= 0 && game->stairY >= 0) {
                    DescendFloor(game);
                }
                if (game->escapeSpawned &&
                    game->player.ent.x == game->escapeX && game->player.ent.y == game->escapeY) {
                    game->state = STATE_WIN;
                }
            }
            return;
        }
    }

    Direction dir = DIR_NONE;

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) dir = DIR_UP;
    else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) dir = DIR_DOWN;
    else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) dir = DIR_LEFT;
    else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) dir = DIR_RIGHT;
    else if (IsKeyPressed(KEY_PERIOD) || IsKeyPressed(KEY_SPACE)) {
        game->sprintBypassRoom = false;
        game->turnCount++;
        game->timeWaited++;
        if (game->timeWaited == 15 && game->currentFloor < game->maxFloors) {
            SpawnShadow(game);
            CombatLog_Add(&game->combatLog, RED, "You feel a presence come to this floor");
        }
        if (game->player.ent.hp < game->player.ent.maxHp) {
            game->player.ent.hp++;
            CombatLog_Add(&game->combatLog, LIGHTGRAY, "Wait heals 1 HP");
        }
        game->enemyTurnCooldown = 0.08f;
        game->state = STATE_ENEMY_TURN;
        return;
    }

    if (dir != DIR_NONE) {
        game->sprintBypassRoom = false;
        int oldX = game->player.ent.x;
        int oldY = game->player.ent.y;
        bool moved = MoveEntity(game, &game->player.ent, dir);
        if (moved) {
            game->selectedMonsterIdx = -1;
            game->turnCount++;
            game->enemyTurnCooldown = 0.15f;
            if (game->player.ent.x != oldX || game->player.ent.y != oldY) {
                game->animTimer = MOVE_ANIM_DURATION;
                game->animDuration = MOVE_ANIM_DURATION;
                RevealFOW(game);
            }
            game->state = STATE_ENEMY_TURN;
        }
    }

    // Open inventory
    if (IsKeyPressed(KEY_I)) {
        game->state = STATE_INVENTORY;
        if (game->inventorySelection < 0 || game->inventorySelection >= game->inventorySlotCount)
            game->inventorySelection = 0;
        return;
    }

    // Check if player is on the stair tile after movement
    if (game->player.ent.x == game->stairX && game->player.ent.y == game->stairY &&
        game->stairX >= 0 && game->stairY >= 0) {
        DescendFloor(game);
    }

    // Check if player is on the escape tile
    if (game->escapeSpawned &&
        game->player.ent.x == game->escapeX && game->player.ent.y == game->escapeY) {
        game->state = STATE_WIN;
    }
}

// Advance flash timers, run monster AI during the enemy turn,
// and check win/loss conditions.
void UpdateGame(Game* game) {
    if (!game) return;

    if (game->animTimer > 0.0f) {
        game->animTimer -= GetFrameTime();
        if (game->animTimer < 0.0f) game->animTimer = 0.0f;
    }
    if (game->monsterAnimTimer > 0.0f) {
        game->monsterAnimTimer -= GetFrameTime();
        if (game->monsterAnimTimer < 0.0f) game->monsterAnimTimer = 0.0f;
    }

    if (game->player.ent.hitFlashTimer > 0.0f) {
        game->player.ent.hitFlashTimer -= GetFrameTime();
        if (game->player.ent.hitFlashTimer < 0.0f) game->player.ent.hitFlashTimer = 0.0f;
    }

    Monster_UpdateAnimations(GetFrameTime());

    if (game->state == STATE_GAME_OVER || game->state == STATE_WIN) return;

    if (game->state == STATE_ENEMY_TURN) {
        if (game->enemyTurnCooldown > 0.0f) {
            game->enemyTurnCooldown -= GetFrameTime();
            return;
        }

        // Process all monster AI
        bool playerAlive = Monster_ProcessAllAI(
            game->player.ent.x, game->player.ent.y, &game->player.ent.hp, game->player.ent.defense,
            &game->player.ent.hitFlashTimer,
            game->blocking, game->map->width, game->map->height,
            &game->combatLog, game->timeWaited);

        if (!playerAlive) {
            game->player.ent.alive = false;
            game->player.ent.hp = 0;
            game->state = STATE_GAME_OVER;
            return;
        }

        if (Monster_GetCount() > 0 && Monster_AreAllDead()) {
            if (game->currentFloor >= game->maxFloors) {
                if (!game->escapeSpawned) {
                    SpawnEscapeTile(game);
                    game->escapeSpawned = true;
                    CombatLog_Add(&game->combatLog, GREEN, "A teleport circle has appeared somewhere for you to escape!");
                }
            } else if (!game->floorClearedNotified) {
                game->floorClearedNotified = true;
                CombatLog_Add(&game->combatLog, YELLOW, "This floor sounds earily Quite now");
            }
        }

        game->monsterAnimTimer = MOVE_ANIM_DURATION;
        game->monsterAnimDuration = MOVE_ANIM_DURATION;
        game->state = STATE_PLAYER_TURN;
    }
}

// Render the map, entities, and HUD.
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

    for (int i = 0; i < game->potionCount; i++) {
        if (game->potionCollected[i]) continue;
        int hx = game->potionTiles[i][0];
        int hy = game->potionTiles[i][1];
        if (game->visibility[hy][hx] != 1) continue;

        Vector2 hpos = TileToScreen(hx, hy, tw, th);
        int pad = tw / 4;
        Color healColor = { 0, 220, 80, 255 };
        DrawRectangle(hpos.x + tw/2 - 2, hpos.y + pad, 4, th - 2*pad, healColor);
        DrawRectangle(hpos.x + pad, hpos.y + th/2 - 2, tw - 2*pad, 4, healColor);
    }

    // --- Interpolation factors ------------------------------------------------
    float pd = (game->animDuration > 0.0f) ? game->animDuration : MOVE_ANIM_DURATION;
    float md = (game->monsterAnimDuration > 0.0f) ? game->monsterAnimDuration : MOVE_ANIM_DURATION;
    float playerT = (game->animTimer <= 0.0f) ? 1.0f : 1.0f - (game->animTimer / pd);
    float monsterT = (game->monsterAnimTimer <= 0.0f) ? 1.0f : 1.0f - (game->monsterAnimTimer / md);

    // --- Monsters ------------------------------------------------------------
    Monster_RenderAll(game->visibility, game->map->width, game->map->height, tw, th, monsterT);

    // --- Player --------------------------------------------------------------
    if (game->player.ent.alive) {
        float px = (float)(game->player.ent.prevX * tw) * (1.0f - playerT) + (float)(game->player.ent.x * tw) * playerT;
        float py = (float)(game->player.ent.prevY * th) * (1.0f - playerT) + (float)(game->player.ent.y * th) * playerT;

        if (game->player.ent.spriteSheet.id > 0) {
            int cellStride = 17; // 16px tile + 1px spacing
            Rectangle src = {
                (float)(game->player.ent.animFrame * cellStride),
                (float)(game->player.ent.spriteRow * cellStride),
                16.0f, 16.0f
            };
            Rectangle dest = { px, py, (float)tw, (float)th };
            Color tint = (game->player.ent.hitFlashTimer > 0.0f) ? (Color){ 255, 255, 255, 200 } : WHITE;
            DrawTexturePro(game->player.ent.spriteSheet, src, dest, (Vector2){ 0, 0 }, 0, tint);
        } else {
            // Fallback: coloured rectangle with face
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

    // --- HUD (screen space) --------------------------------------------------

    // Stats panel (bottom-left)
    int panelX = 10;
    int panelW = 260;
    int barW = panelW - 20;
    int barH = 16;
    int textY = 0;

    // Level
    char levelText[64];
    snprintf(levelText, sizeof(levelText), "Lv %d", game->player.ent.level);
    int panelH = 100;
    int panelY = GetScreenHeight() - 10 - panelH;
    DrawRectangle(panelX - 4, panelY, panelW, panelH, (Color){ 0, 0, 0, 180 });
    textY = panelY + 4;
    DrawText(levelText, panelX, textY, 18, WHITE);

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

    // Floor + enemies
    textY += barH + 6;
    char infoText[64];
    snprintf(infoText, sizeof(infoText), "Floor: %d/%d", game->currentFloor, game->maxFloors);
    DrawText(infoText, panelX, textY, 14, LIGHTGRAY);

    // Combat log (bottom-right)
    CombatLog_Render(&game->combatLog, GetScreenWidth() - 370, GetScreenHeight() - 10, 14, 18);

    MonsterInfo_Render(game);

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

    // --- Inventory overlay ---------------------------------------------------
    if (game->state == STATE_INVENTORY) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        int iw = 400;
        int ih = 300;
        int ix = (sw - iw) / 2;
        int iy = (sh - ih) / 2;

        DrawRectangle(0, 0, sw, sh, (Color){ 0, 0, 0, 160 });
        DrawRectangle(ix, iy, iw, ih, (Color){ 30, 30, 40, 255 });
        DrawRectangleLines(ix, iy, iw, ih, LIGHTGRAY);

        int textX = ix + 16;
        int textY = iy + 12;
        DrawText("INVENTORY", textX, textY, 20, YELLOW);
        textY += 30;

        if (game->inventorySlotCount == 0) {
            DrawText("(empty)", textX, textY, 16, GRAY);
        } else {
            for (int i = 0; i < game->inventorySlotCount; i++) {
                Color c = (i == game->inventorySelection) ? YELLOW : WHITE;
                char line[128];
                snprintf(line, sizeof(line), "%s x%d", GetItemName(game->inventory[i].type), game->inventory[i].quantity);
                if (i == game->inventorySelection)
                    DrawText(">", textX - 18, textY, 16, YELLOW);
                DrawText(line, textX, textY, 16, c);
                textY += 22;
            }
            DrawText("ENTER to use  |  I / ESC to close", ix + 16, iy + ih - 28, 14, GRAY);
        }
    }
}

// Build the blocking grid from the tile map.
// Looks for a layer named "collision" (case-sensitive); if not found,
// uses layer index 1 as the collision layer.  Any non-zero GID blocks movement.
static void BuildBlockingMap(Game* game) {
    if (!game || !game->map) return;

    int w = game->map->width;
    int h = game->map->height;

    int collisionLayer = -1;
    for (int i = 0; i < game->map->layerCount; i++) {
        if (strcmp(game->map->layers[i].name, "collision") == 0 ||
            strcmp(game->map->layers[i].name, "Collision") == 0) {
            collisionLayer = i;
            break;
        }
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (collisionLayer >= 0) {
                game->blocking[y][x] = (GetTileGID(game->map, collisionLayer, x, y) > 0) ? 1 : 0;
            } else if (game->map->layerCount >= 2) {
                game->blocking[y][x] = (GetTileGID(game->map, 1, x, y) > 0) ? 1 : 0;
            } else {
                game->blocking[y][x] = 0;
            }
        }
    }
}

// Iterate map objects from the TMX file and spawn the player,
// health potions, or monsters based on the object's type field.
static void SpawnEntitiesFromObjects(Game* game) {
    if (!game || !game->map) return;

    for (int i = 0; i < game->map->objectCount; i++) {
        MapObject* obj = &game->map->objects[i];
        int tileX = (int)(obj->x / game->map->tileWidth);
        int tileY = (int)(obj->y / game->map->tileHeight);

        // --- Player spawn ----------------------------------------------------
        if (strcmp(obj->type, "player") == 0 || strcmp(obj->name, "player") == 0 ||
            strcmp(obj->type, "Player") == 0) {
            game->player.ent.x = tileX;
            game->player.ent.y = tileY;
            game->player.ent.prevX = tileX;
            game->player.ent.prevY = tileY;
            TraceLog(LOG_INFO, "Player spawned at (%d, %d)", tileX, tileY);
        }
        // --- Health potion ---------------------------------------------------
        else if (strcmp(obj->type, "healing") == 0 || strcmp(obj->type, "Healing") == 0 ||
                 strcmp(obj->type, "health") == 0 || strcmp(obj->type, "Health") == 0 ||
                 strcmp(obj->type, "health_potion") == 0 ||
                 strcmp(obj->name, "health_potion") == 0 || strcmp(obj->name, "HealthPotion") == 0) {
            if (game->potionCount >= MAX_POTIONS) continue;
            game->potionTiles[game->potionCount][0] = tileX;
            game->potionTiles[game->potionCount][1] = tileY;
            game->potionCollected[game->potionCount] = false;
            if (game->currentFloor <= 2)      game->potionTypes[game->potionCount] = ITEM_SMALL_HP_POTION;
            else if (game->currentFloor <= 5) game->potionTypes[game->potionCount] = ITEM_BIG_HP_POTION;
            else                              game->potionTypes[game->potionCount] = ITEM_LARGE_HP_POTION;
            game->potionCount++;
            TraceLog(LOG_INFO, "Health potion at (%d, %d)", tileX, tileY);
        }
        // --- Monsters --------------------------------------------------------
        // TMX object type is matched against MonsterTemplate.tmxTypeName
        else {
            Monster* mon = Monster_SpawnByTypeName(obj->type, tileX, tileY, game->currentFloor);
            if (mon) TraceLog(LOG_INFO, "%s spawned at (%d, %d)", mon->name, tileX, tileY);
        }
    }
}

// Regenerate the map when the player descends to the next floor.
// Player stats (HP, level, EXP) are preserved; the map and entities are rebuilt.
void DescendFloor(Game* game) {
    Player savedPlayer = game->player;
    InventorySlot savedInventory[MAX_INVENTORY_SLOTS];
    memcpy(savedInventory, game->inventory, sizeof(savedInventory));
    int savedInvCount = game->inventorySlotCount;

    Monster_UnloadSprites();
    Monster_ResetAll();

    if (game->map) {
        for (int t = 0; t < game->map->tilesetCount; t++) {
            if (game->tilesetTextures[t].id > 0)
                UnloadTexture(game->tilesetTextures[t]);
        }
        UnloadTMX(game->map);
    }

    int floor = game->currentFloor + 1;

    memset(game, 0, sizeof(Game));
    game->player = savedPlayer;
    memcpy(game->inventory, savedInventory, sizeof(savedInventory));
    game->inventorySlotCount = savedInvCount;
    game->selectedMonsterIdx = -1;
    game->currentFloor = floor;
    game->maxFloors = 10;
    game->player.ent.spriteSheet = LoadTexture("resources/roguelikeChar_transparent.png");
    if (game->player.ent.spriteSheet.id == 0) {
        TraceLog(LOG_WARNING, "Could not load player spritesheet during descend");
    }

    game->map = GenerateProceduralMap(80, 50, game->currentFloor < game->maxFloors);
    if (!game->map) {
        TraceLog(LOG_ERROR, "Failed to generate map for floor %d", game->currentFloor);
        return;
    }

    BuildBlockingMap(game);

    for (int t = 0; t < game->map->tilesetCount; t++) {
        char imgPath[512] = {0};
        if (imgPath[0] == '\0' || !FileExists(imgPath)) {
            snprintf(imgPath, sizeof(imgPath), "resources/%s", game->map->tilesets[t].imageSource);
        }
        game->tilesetTextures[t] = LoadTexture(imgPath);
        if (game->tilesetTextures[t].id == 0) {
            TraceLog(LOG_WARNING, "Could not load tileset texture: %s", imgPath);
            if (t == 0) {
                Image img = GenImageColor(game->map->tileWidth * 8, game->map->tileHeight * 8,
                                          (Color){ 100, 100, 100, 255 });
                for (int x = 0; x < 8; x++) {
                    for (int y = 0; y < 8; y++) {
                        Color c = ((x + y) % 2 == 0)
                            ? (Color){ 120, 120, 120, 255 }
                            : (Color){ 80, 80, 80, 255 };
                        ImageDrawRectangle(&img, x * game->map->tileWidth, y * game->map->tileHeight,
                                           game->map->tileWidth - 1, game->map->tileHeight - 1, c);
                    }
                }
                game->tilesetTextures[t] = LoadTextureFromImage(img);
                UnloadImage(img);
                game->map->tilesets[0].columns = 8;
                game->map->tilesets[0].imageWidth = game->map->tileWidth * 8;
                game->map->tilesets[0].imageHeight = game->map->tileHeight * 8;
            }
        }
    }

    Monster_InitTemplates();

    SpawnEntitiesFromObjects(game);

    ProceduralRoom spawnRooms[MAX_GENERATED_ROOMS];
    int spawnRoomCount = GetGeneratedRooms(spawnRooms, MAX_GENERATED_ROOMS);
    if (spawnRoomCount > 0) {
        Spawner_Populate(game, spawnRooms, spawnRoomCount);
    }

    Monster_LoadSprites();

    game->stairX = GetStairX();
    game->stairY = GetStairY();

    game->state = STATE_PLAYER_TURN;
    game->turnCount = 0;
    game->enemyTurnCooldown = 0.0f;
    game->animTimer = 0.0f;
    game->monsterAnimTimer = 0.0f;
    game->animDuration = 0.0f;
    game->monsterAnimDuration = 0.0f;
    game->sprintBypassRoom = false;

    for (int y = 0; y < game->map->height; y++) {
        for (int x = 0; x < game->map->width; x++) {
            game->visibility[y][x] = 0;
        }
    }
    RevealFOW(game);

    game->camera.target = (Vector2){
        game->player.ent.x * game->map->tileWidth  + game->map->tileWidth  / 2,
        game->player.ent.y * game->map->tileHeight + game->map->tileHeight / 2
    };
    game->camera.offset = (Vector2){ GetScreenWidth() / 2, GetScreenHeight() / 2 };
    game->camera.rotation = 0;
    game->camera.zoom = 4.0f;

    char floorMsg[64];
    snprintf(floorMsg, sizeof(floorMsg), "You descend to floor %d", game->currentFloor);
    CombatLog_Add(&game->combatLog, LIGHTGRAY, floorMsg);
    TraceLog(LOG_INFO, "%s", floorMsg);
}

// ---------------------------------------------------------------------------
// Item metadata
// ---------------------------------------------------------------------------
static const char* ITEM_NAMES[ITEM_COUNT] = {
    "",
    "Small HP Potion",
    "Big HP Potion",
    "Large HP Potion"
};
static const int ITEM_HEALS[ITEM_COUNT] = {
    0,
    8,  // floor 1-2
    18, // floor 3-5
    36  // floor 6+
};

const char* GetItemName(ItemType type) {
    if (type < 0 || type >= ITEM_COUNT) return "";
    return ITEM_NAMES[type];
}

int GetItemHealAmount(ItemType type) {
    if (type < 0 || type >= ITEM_COUNT) return 0;
    return ITEM_HEALS[type];
}

// Add one instance of the given item to the inventory (stacking).
// Returns true if added, false if inventory is full.
bool InventoryAdd(Game* game, ItemType type) {
    if (type == ITEM_NONE) return false;
    // Try to stack on an existing slot of the same type
    for (int i = 0; i < game->inventorySlotCount; i++) {
        if (game->inventory[i].type == type) {
            game->inventory[i].quantity++;
            return true;
        }
    }
    // Need a new slot
    if (game->inventorySlotCount >= MAX_INVENTORY_SLOTS) return false;
    game->inventory[game->inventorySlotCount].type = type;
    game->inventory[game->inventorySlotCount].quantity = 1;
    game->inventorySlotCount++;
    return true;
}

// Use (consume) the item in the given inventory slot.
// Returns true if used successfully.
bool InventoryUse(Game* game, int slot) {
    if (slot < 0 || slot >= game->inventorySlotCount) return false;
    InventorySlot* s = &game->inventory[slot];
    if (s->type == ITEM_NONE || s->quantity <= 0) return false;

    int heal = GetItemHealAmount(s->type);
    if (heal > 0) {
        game->player.ent.hp += heal;
        if (game->player.ent.hp > game->player.ent.maxHp)
            game->player.ent.hp = game->player.ent.maxHp;
        CombatLog_Add(&game->combatLog, LIGHTGRAY, "Used %s — restores %d HP!", GetItemName(s->type), heal);
    }

    s->quantity--;
    if (s->quantity <= 0) {
        // Remove slot by shifting later ones down
        for (int i = slot; i < game->inventorySlotCount - 1; i++)
            game->inventory[i] = game->inventory[i + 1];
        game->inventorySlotCount--;
        if (game->inventorySelection >= game->inventorySlotCount)
            game->inventorySelection = game->inventorySlotCount - 1;
    }
    return true;
}

// Initialise (or re-initialise) the game: load map, build blocking,
// load tileset, spawn entities, set default player stats,
// and configure the camera.
bool InitGame(Game* game, const char* tmxFile) {
    if (!game) return false;
    memset(game, 0, sizeof(Game));

    // Load TMX map
    game->map = LoadTMX(tmxFile);
    if (!game->map) {
        TraceLog(LOG_INFO, "TMX load failed, generating procedural map...");
        game->map = GenerateProceduralMap(80, 50, 1);
    }
    if (!game->map) {
        TraceLog(LOG_ERROR, "Failed to create any map");
        return false;
    }

    BuildBlockingMap(game);

    // Try to load all tileset textures
    for (int t = 0; t < game->map->tilesetCount; t++) {
        char imgPath[512] = {0};
        if (tmxFile) {
            const char* lastSlash = strrchr(tmxFile, '/');
            const char* lastBackslash = strrchr(tmxFile, '\\');
            const char* sep = (lastSlash > lastBackslash) ? lastSlash : lastBackslash;
            if (sep) {
                int dirLen = (int)(sep - tmxFile) + 1;
                snprintf(imgPath, sizeof(imgPath), "%.*s%s", dirLen, tmxFile, game->map->tilesets[t].imageSource);
            } else {
                snprintf(imgPath, sizeof(imgPath), "%s", game->map->tilesets[t].imageSource);
            }
        }
        if (imgPath[0] == '\0' || !FileExists(imgPath)) {
            snprintf(imgPath, sizeof(imgPath), "resources/%s", game->map->tilesets[t].imageSource);
        }

        TraceLog(LOG_INFO, "Loading tileset texture [%d]: %s", t, imgPath);
        game->tilesetTextures[t] = LoadTexture(imgPath);

        if (game->tilesetTextures[t].id == 0) {
            TraceLog(LOG_WARNING, "Could not load tileset texture: %s", imgPath);
            if (t == 0) {
                Image img = GenImageColor(game->map->tileWidth * 8, game->map->tileHeight * 8,
                                          (Color){ 100, 100, 100, 255 });
                for (int x = 0; x < 8; x++) {
                    for (int y = 0; y < 8; y++) {
                        Color c = ((x + y) % 2 == 0)
                            ? (Color){ 120, 120, 120, 255 }
                            : (Color){ 80, 80, 80, 255 };
                        ImageDrawRectangle(&img, x * game->map->tileWidth, y * game->map->tileHeight,
                                           game->map->tileWidth - 1, game->map->tileHeight - 1, c);
                    }
                }
                game->tilesetTextures[t] = LoadTextureFromImage(img);
                UnloadImage(img);
                game->map->tilesets[0].columns = 8;
                game->map->tilesets[0].imageWidth  = game->map->tileWidth * 8;
                game->map->tilesets[0].imageHeight = game->map->tileHeight * 8;
            }
        }
    }

    // Player defaults (overridden by map objects below)
    game->player.ent.x = 1;
    game->player.ent.y = 1;
    game->player.ent.prevX = 1;
    game->player.ent.prevY = 1;
    strcpy(game->player.ent.name, "Hero");
    game->player.ent.hp = 20;
    game->player.ent.maxHp = 20;
    game->player.ent.attack = 5;
    game->player.ent.defense = 1;
    game->player.ent.level = 1;
    game->player.ent.alive = true;
    game->player.ent.isPlayer = true;
    game->player.ent.facingRight = true;
    game->player.ent.color = (Color){ 50, 200, 255, 255 };
    game->player.ent.spriteRow = 6;

    game->player.ent.spriteSheet = LoadTexture("resources/roguelikeChar_transparent.png");
    if (game->player.ent.spriteSheet.id == 0) {
        TraceLog(LOG_WARNING, "Could not load player spritesheet, using fallback rendering");
    }

    game->player.exp = 0;
    game->player.expToNext = ExpForLevel(1);

    game->currentFloor = 1;

    Monster_InitTemplates();

    SpawnEntitiesFromObjects(game);

    ProceduralRoom spawnRooms[MAX_GENERATED_ROOMS];
    int spawnRoomCount = GetGeneratedRooms(spawnRooms, MAX_GENERATED_ROOMS);
    if (spawnRoomCount > 0) {
        Spawner_Populate(game, spawnRooms, spawnRoomCount);
    }

    // Load monster sprite sheets
    Monster_LoadSprites();

    game->selectedMonsterIdx = -1;
    game->timeWaited = 0;
    game->escapeSpawned = false;
    game->maxFloors = 10;
    game->stairX = GetStairX();
    game->stairY = GetStairY();

    game->state = STATE_PLAYER_TURN;
    game->turnCount = 0;
    game->enemyTurnCooldown = 0.0f;
    game->animTimer = 0.0f;
    game->monsterAnimTimer = 0.0f;
    game->animDuration = 0.0f;
    game->monsterAnimDuration = 0.0f;

    for (int y = 0; y < game->map->height; y++) {
        for (int x = 0; x < game->map->width; x++) {
            game->visibility[y][x] = 0;
        }
    }
    RevealFOW(game);

    game->camera.target = (Vector2){
        game->player.ent.x * game->map->tileWidth  + game->map->tileWidth  / 2,
        game->player.ent.y * game->map->tileHeight + game->map->tileHeight / 2
    };
    game->camera.offset = (Vector2){ GetScreenWidth() / 2, GetScreenHeight() / 2 };
    game->camera.rotation = 0;
    game->camera.zoom = 4.0f;

    return true;
}

// Free all runtime assets (sprites, map, textures) and zero the Game struct.
// Safe to call on an already-cleaned-up Game (idempotent if map is NULL).
void CleanupGame(Game* game) {
    if (!game) return;

    Monster_UnloadSprites();
    Monster_ResetAll();

    if (game->map) {
        for (int t = 0; t < game->map->tilesetCount; t++) {
            if (game->tilesetTextures[t].id > 0)
                UnloadTexture(game->tilesetTextures[t]);
        }
    }
    if (game->player.ent.spriteSheet.id > 0) UnloadTexture(game->player.ent.spriteSheet);
    if (game->map) {
        UnloadTMX(game->map);
        game->map = NULL;
    }

    memset(game, 0, sizeof(Game));
}
