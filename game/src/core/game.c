#include "game.h"
#include "entity/entity.h"
#include "entity/player.h"
#include "ui/combat_log.h"
#include "entity/monster.h"
#include "procedural.h"
#include "entity/spawner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple circular radius reveal: all tiles within FOG_RADIUS are lit.
// Previously-seen tiles are dimmed. Walls do not block sight.
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

// Read directional keys (arrow keys / WASD) and pass them to MoveEntity.
// Period/space waits a turn, restoring 1 HP.
// After any action, control passes to the enemy turn.
void HandleInput(Game* game) {
    if (game->state != STATE_PLAYER_TURN) return;

    Direction dir = DIR_NONE;

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) dir = DIR_UP;
    else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) dir = DIR_DOWN;
    else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) dir = DIR_LEFT;
    else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) dir = DIR_RIGHT;
    else if (IsKeyPressed(KEY_PERIOD) || IsKeyPressed(KEY_SPACE)) {
        game->turnCount++;
        if (game->player.ent.hp < game->player.ent.maxHp) {
            game->player.ent.hp++;
            CombatLog_Add(&game->combatLog, "Wait heals 1 HP");
        }
        game->enemyTurnCooldown = 0.08f;
        game->state = STATE_ENEMY_TURN;
        return;
    }

    if (dir != DIR_NONE) {
        int oldX = game->player.ent.x;
        int oldY = game->player.ent.y;
        bool moved = MoveEntity(game, &game->player.ent, dir);
        if (moved) {
            game->turnCount++;
            game->enemyTurnCooldown = 0.15f;
            if (game->player.ent.x != oldX || game->player.ent.y != oldY) {
                game->animTimer = MOVE_ANIM_DURATION;
                RevealFOW(game);
            }
            game->state = STATE_ENEMY_TURN;
        }
    }
}

// Advance flash timers, run monster AI if it is the enemy turn,
// and check win / loss conditions.
void UpdateGame(Game* game) {
    if (!game) return;

    // Decrement animation timers unconditionally
    if (game->animTimer > 0.0f) {
        game->animTimer -= GetFrameTime();
        if (game->animTimer < 0.0f) game->animTimer = 0.0f;
    }
    if (game->monsterAnimTimer > 0.0f) {
        game->monsterAnimTimer -= GetFrameTime();
        if (game->monsterAnimTimer < 0.0f) game->monsterAnimTimer = 0.0f;
    }

    // Player flash timer countdown
    if (game->player.ent.hitFlashTimer > 0.0f) {
        game->player.ent.hitFlashTimer -= GetFrameTime();
        if (game->player.ent.hitFlashTimer < 0.0f) game->player.ent.hitFlashTimer = 0.0f;
    }

    Monster_UpdateAnimations(GetFrameTime());

    if (game->state == STATE_GAME_OVER || game->state == STATE_WIN) return;

    // --- Enemy turn -----------------------------------------------------------
    if (game->state == STATE_ENEMY_TURN) {
        // Brief pause before monsters act so the player's hit sound plays first
        if (game->enemyTurnCooldown > 0.0f) {
            game->enemyTurnCooldown -= GetFrameTime();
            return;
        }

        // Process all monster AI
        bool playerAlive = Monster_ProcessAllAI(
            game->player.ent.x, game->player.ent.y, &game->player.ent.hp, game->player.ent.defense,
            &game->player.ent.hitFlashTimer,
            game->blocking, game->map->width, game->map->height,
            &game->combatLog);

        if (!playerAlive) {
            game->player.ent.alive = false;
            game->player.ent.hp = 0;
            game->state = STATE_GAME_OVER;
            return;
        }

        if (Monster_GetCount() > 0 && Monster_AreAllDead()) {
            game->state = STATE_WIN;
            return;
        }

        game->monsterAnimTimer = MOVE_ANIM_DURATION;
        game->state = STATE_PLAYER_TURN;
    }
}

// Render all visible tiles, healing items, monsters, the player,
// and the HUD overlay.
void RenderGame(const Game* game) {
    if (!game || !game->map) return;

    int tw = game->map->tileWidth;
    int th = game->map->tileHeight;

    BeginMode2D(game->camera);

    // --- Tile layers ---------------------------------------------------------
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

    // --- Healing items -------------------------------------------------------
    for (int i = 0; i < game->healingCount; i++) {
        if (game->healingCollected[i]) continue;
        int hx = game->healingTiles[i][0];
        int hy = game->healingTiles[i][1];
        if (game->visibility[hy][hx] != 1) continue;

        Vector2 hpos = TileToScreen(hx, hy, tw, th);
        int pad = tw / 4;
        Color healColor = { 0, 220, 80, 255 };
        DrawRectangle(hpos.x + tw/2 - 2, hpos.y + pad, 4, th - 2*pad, healColor);
        DrawRectangle(hpos.x + pad, hpos.y + th/2 - 2, tw - 2*pad, 4, healColor);
    }

    // --- Interpolation factors ------------------------------------------------
    float playerT = (game->animTimer <= 0.0f) ? 1.0f : 1.0f - (game->animTimer / MOVE_ANIM_DURATION);
    float monsterT = (game->monsterAnimTimer <= 0.0f) ? 1.0f : 1.0f - (game->monsterAnimTimer / MOVE_ANIM_DURATION);

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

    // Turn + enemies
    textY += barH + 6;
    char infoText[64];
    snprintf(infoText, sizeof(infoText), "Turn: %d   Enemies: %d", game->turnCount, Monster_GetAliveCount());
    DrawText(infoText, panelX, textY, 14, LIGHTGRAY);

    // Combat log (bottom-right)
    CombatLog_Render(&game->combatLog, GetScreenWidth() - 370, GetScreenHeight() - 10, 14, 18);

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
// healing pickups, or monsters based on the object's type field.
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
        // --- Healing item ----------------------------------------------------
        else if (strcmp(obj->type, "healing") == 0 || strcmp(obj->type, "Healing") == 0 ||
                 strcmp(obj->type, "health") == 0 || strcmp(obj->type, "Health") == 0) {
            if (game->healingCount >= MAX_HEALING) continue;
            game->healingTiles[game->healingCount][0] = tileX;
            game->healingTiles[game->healingCount][1] = tileY;
            game->healingCollected[game->healingCount] = false;
            game->healingCount++;
            TraceLog(LOG_INFO, "Healing item at (%d, %d)", tileX, tileY);
        }
        // --- Monsters --------------------------------------------------------
        // TMX object type is matched against MonsterTemplate.tmxTypeName
        else {
            Monster* mon = Monster_SpawnByTypeName(obj->type, tileX, tileY);
            if (mon) TraceLog(LOG_INFO, "%s spawned at (%d, %d)", mon->name, tileX, tileY);
        }
    }
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
        game->map = GenerateProceduralMap(80, 50);
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

    // Initialize player defaults (overridden by map objects below)
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

    // Load player character spritesheet
    game->player.ent.spriteSheet = LoadTexture("resources/roguelikeChar_transparent.png");
    if (game->player.ent.spriteSheet.id == 0) {
        TraceLog(LOG_WARNING, "Could not load player spritesheet, using fallback rendering");
    }

    // Player-specific fields (exp / leveling)
    game->player.exp = 0;
    game->player.expToNext = ExpForLevel(1);

    // Initialize monster templates (must be ready before spawning)
    Monster_InitTemplates();

    // Spawn entities from map objects
    SpawnEntitiesFromObjects(game);

    // Populate procedural maps with monsters and items
    ProceduralRoom spawnRooms[MAX_GENERATED_ROOMS];
    int spawnRoomCount = GetGeneratedRooms(spawnRooms, MAX_GENERATED_ROOMS);
    if (spawnRoomCount > 0) {
        Spawner_Populate(game, spawnRooms, spawnRoomCount);
    }

    // Load monster sprite sheets
    Monster_LoadSprites();

    // Game state
    game->state = STATE_PLAYER_TURN;
    game->turnCount = 0;
    game->enemyTurnCooldown = 0.0f;
    game->animTimer = 0.0f;
    game->monsterAnimTimer = 0.0f;

    // Fog of war: start unexplored, reveal starting area
    for (int y = 0; y < game->map->height; y++) {
        for (int x = 0; x < game->map->width; x++) {
            game->visibility[y][x] = 0;
        }
    }
    RevealFOW(game);

    // Setup camera
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
