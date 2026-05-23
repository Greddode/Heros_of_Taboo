#include "game.h"
#include "monster.h"
#include "procedural.h"
#include "spawner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Tile <-> Screen coordinate conversion
// ---------------------------------------------------------------------------
Vector2 TileToScreen(int x, int y, int tileWidth, int tileHeight) {
    return (Vector2){ (float)(x * tileWidth), (float)(y * tileHeight) };
}

// ---------------------------------------------------------------------------
// Walkability check
// ---------------------------------------------------------------------------
bool IsWalkable(const Game* game, int x, int y) {
    if (!game->map) return false;
    if (x < 0 || x >= game->map->width || y < 0 || y >= game->map->height) return false;
    if (game->blocking[y][x]) return false;
    // Check if a monster occupies the tile
    Monster* m = Monster_GetAt(x, y, NULL);
    if (m && m->alive) return false;
    return true;
}

// ---------------------------------------------------------------------------
// Find entity at a tile position (player only; monsters handled separately)
// ---------------------------------------------------------------------------
Entity* GetEntityAt(Game* game, int x, int y, Entity* exclude) {
    if (!exclude || !exclude->isPlayer) {
        if (game->player.alive && game->player.x == x && game->player.y == y) {
            return &game->player;
        }
    }
    return NULL;
}

// ---------------------------------------------------------------------------
// Move an entity in a direction. Returns true if an action was taken.
// Combat: the hitting entity deals damage to the target.
// ---------------------------------------------------------------------------
bool MoveEntity(Game* game, Entity* entity, Direction dir) {
    if (!entity->alive) return false;

    if (dir == DIR_LEFT)  entity->facingRight = false;
    if (dir == DIR_RIGHT) entity->facingRight = true;

    int newX = entity->x;
    int newY = entity->y;

    switch (dir) {
        case DIR_UP:    newY--; break;
        case DIR_DOWN:  newY++; break;
        case DIR_LEFT:  newX--; break;
        case DIR_RIGHT: newX++; break;
        default: return false;
    }

    // Bounds check
    if (newX < 0 || newX >= game->map->width ||
        newY < 0 || newY >= game->map->height) return false;

    // Check if there's an entity at the target (player vs player — rare)
    Entity* target = GetEntityAt(game, newX, newY, entity);
    if (target) {
        int damage = entity->attack - target->defense;
        if (damage < 1) damage = 1;
        target->hp -= damage;
        TraceLog(LOG_INFO, "%s attacks %s for %d damage! (HP: %d/%d)",
                 entity->name, target->name, damage, target->hp, target->maxHp);
        if (target->hp <= 0) {
            target->alive = false;
            target->hp = 0;
            TraceLog(LOG_INFO, "%s has been slain!", target->name);
        }
        return true;
    }

    // Player attacking a monster
    if (entity->isPlayer) {
        Monster* mon = Monster_GetAt(newX, newY, NULL);
        if (mon && mon->alive) {
            int damage = entity->attack - mon->defense;
            if (damage < 1) damage = 1;
            mon->hp -= damage;
            entity->hitFlashTimer = 0.15f;
            mon->hitFlashTimer = 0.15f;
            TraceLog(LOG_INFO, "%s attacks %s for %d damage! (HP: %d/%d)",
                     entity->name, mon->name, damage, mon->hp, mon->maxHp);
            if (mon->hp <= 0) {
                mon->alive = false;
                entity->expValue += mon->expValue;
                TraceLog(LOG_INFO, "%s has been slain!", mon->name);
            }
            return true;
        }
    }

    // Blocked by wall
    if (game->blocking[newY][newX]) return false;

    // Move
    entity->prevX = entity->x;
    entity->prevY = entity->y;
    entity->x = newX;
    entity->y = newY;

    // Healing pickup (player only)
    if (entity->isPlayer) {
        for (int h = 0; h < game->healingCount; h++) {
            if (!game->healingCollected[h] &&
                game->healingTiles[h][0] == newX &&
                game->healingTiles[h][1] == newY) {
                game->healingCollected[h] = true;
                int healAmt = 8;
                game->player.hp += healAmt;
                if (game->player.hp > game->player.maxHp)
                    game->player.hp = game->player.maxHp;
                TraceLog(LOG_INFO, "Picked up healing! HP: %d/%d",
                         game->player.hp, game->player.maxHp);
            }
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// Simple recursive shadowcasting FOV
// ---------------------------------------------------------------------------
static void CastLight(Game* game, int cx, int cy, int radius, int row,
                      float startSlope, float endSlope,
                      int xx, int xy, int yx, int yy) {
    if (startSlope < endSlope) return;

    for (int j = row; j <= radius; j++) {
        bool blocked = false;
        for (int i = -j; i <= 0; i++) {
            int x = cx + i * xx + j * xy;
            int y = cy + i * yx + j * yy;

            float leftSlope  = (float)(i - 0.5f) / (float)(j + 0.5f);
            float rightSlope = (float)(i + 0.5f) / (float)(j - 0.5f);

            if (x < 0 || x >= game->map->width ||
                y < 0 || y >= game->map->height) continue;
            if ((float)(i * i + j * j) > (float)(radius * radius)) continue;

            if (startSlope < rightSlope) continue;
            if (endSlope > leftSlope) break;

            if (game->visibility[y][x] == 0) game->visibility[y][x] = 1;

            if (blocked) {
                if (game->blocking[y][x]) { blocked = true; }
                else { blocked = false; startSlope = rightSlope; }
            } else if (game->blocking[y][x] && j < radius) {
                blocked = true;
                CastLight(game, cx, cy, radius, j + 1,
                          startSlope, leftSlope, xx, xy, yx, yy);
                startSlope = rightSlope;
            }
        }
        if (blocked) break;
    }
}

void ComputeFOV(Game* game, int px, int py, int radius) {
    if (!game) return;

    for (int y = 0; y < game->map->height; y++) {
        for (int x = 0; x < game->map->width; x++) {
            if (game->visibility[y][x] == 1) game->visibility[y][x] = 2;
        }
    }

    if (px >= 0 && px < game->map->width &&
        py >= 0 && py < game->map->height) {
        game->visibility[py][px] = 1;
    }

    int cx = px, cy = py;
    CastLight(game, cx, cy, radius, 1, 1.0f, 0.0f,  1, 0, 0, 1);
    CastLight(game, cx, cy, radius, 1, 1.0f, 0.0f,  0, 1, 1, 0);
    CastLight(game, cx, cy, radius, 1, 1.0f, 0.0f, -1, 0, 0, 1);
    CastLight(game, cx, cy, radius, 1, 1.0f, 0.0f,  0,-1, 1, 0);
    CastLight(game, cx, cy, radius, 1, 1.0f, 0.0f, -1, 0, 0,-1);
    CastLight(game, cx, cy, radius, 1, 1.0f, 0.0f,  0,-1,-1, 0);
    CastLight(game, cx, cy, radius, 1, 1.0f, 0.0f,  1, 0, 0,-1);
    CastLight(game, cx, cy, radius, 1, 1.0f, 0.0f,  0, 1,-1, 0);
}

// ---------------------------------------------------------------------------
// Handle player input
// ---------------------------------------------------------------------------
void HandleInput(Game* game) {
    if (game->state != STATE_PLAYER_TURN) return;

    Direction dir = DIR_NONE;

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) dir = DIR_UP;
    else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) dir = DIR_DOWN;
    else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) dir = DIR_LEFT;
    else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) dir = DIR_RIGHT;
    else if (IsKeyPressed(KEY_PERIOD) || IsKeyPressed(KEY_SPACE)) {
        game->turnCount++;
        if (game->player.hp < game->player.maxHp) game->player.hp++;
        game->state = STATE_ENEMY_TURN;
        return;
    }

    if (dir != DIR_NONE) {
        bool moved = MoveEntity(game, &game->player, dir);
        if (moved) {
            game->turnCount++;
            game->state = STATE_ENEMY_TURN;
        }
    }
}

// ---------------------------------------------------------------------------
// Update game logic
// ---------------------------------------------------------------------------
void UpdateGame(Game* game) {
    if (!game) return;

    // Player flash timer countdown
    if (game->player.hitFlashTimer > 0.0f) {
        game->player.hitFlashTimer -= GetFrameTime();
        if (game->player.hitFlashTimer < 0.0f) game->player.hitFlashTimer = 0.0f;
    }

    Monster_UpdateAnimations(GetFrameTime());

    if (game->state == STATE_GAME_OVER || game->state == STATE_WIN) return;

    // --- Enemy turn -----------------------------------------------------------
    if (game->state == STATE_ENEMY_TURN) {
        // Process all monster AI
        bool playerAlive = Monster_ProcessAllAI(
            game->player.x, game->player.y, &game->player.hp, game->player.defense,
            &game->player.hitFlashTimer,
            game->blocking, game->map->width, game->map->height);

        if (!playerAlive) {
            game->player.alive = false;
            game->player.hp = 0;
            game->state = STATE_GAME_OVER;
            return;
        }

        if (Monster_GetCount() > 0 && Monster_AreAllDead()) {
            game->state = STATE_WIN;
            return;
        }

        // Switch back to player turn
        game->state = STATE_PLAYER_TURN;
    }
}

// ---------------------------------------------------------------------------
// Draw a single tile
// ---------------------------------------------------------------------------
void DrawTile(const Game* game, int x, int y, int layerIndex) {
    if (!game->map || !game->map->layers) return;

    int gid = GetTileGID(game->map, layerIndex, x, y);
    if (gid <= 0) return;

    Rectangle src = GetTileSourceRect(game->map, gid);
    if (src.width <= 0 || src.height <= 0) return;

    Vector2 pos = TileToScreen(x, y, game->map->tileWidth, game->map->tileHeight);
    Rectangle dest = { pos.x, pos.y, (float)game->map->tileWidth, (float)game->map->tileHeight };

    DrawTexturePro(game->tilesetTexture, src, dest, (Vector2){0, 0}, 0, WHITE);
}

// ---------------------------------------------------------------------------
// Render the game
// ---------------------------------------------------------------------------
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

    // --- Monsters ------------------------------------------------------------
    Monster_RenderAll(game->visibility, game->map->width, game->map->height, tw, th, -1.0f);

    // --- Player --------------------------------------------------------------
    if (game->player.alive) {
        float px = (float)(game->player.x * tw);
        float py = (float)(game->player.y * th);

        int pad = tw / 6;
        Color playerColor = (game->player.hitFlashTimer > 0.0f) ? WHITE : game->player.color;
        DrawRectangle(px + pad, py + pad, tw - 2*pad, th - 2*pad, playerColor);

        int cx = px + tw / 2;
        int cy = py + th / 2;
        DrawCircle(cx - 3, cy - 2, 2, WHITE);
        DrawCircle(cx + 3, cy - 2, 2, WHITE);

        // Simple mouth
        if (game->player.facingRight) {
            DrawCircle(cx + 5, cy + 4, 1, WHITE);
        } else {
            DrawCircle(cx - 5, cy + 4, 1, WHITE);
        }
    }

    EndMode2D();

    // --- HUD (screen space) --------------------------------------------------
    char turnText[64];
    snprintf(turnText, sizeof(turnText), "Turn: %d", game->turnCount);
    DrawText(turnText, 10, 10, 20, WHITE);

    char hpText[64];
    snprintf(hpText, sizeof(hpText), "HP: %d/%d", game->player.hp, game->player.maxHp);
    DrawText(hpText, 10, 35, 20, WHITE);

    char enemyText[64];
    snprintf(enemyText, sizeof(enemyText), "Enemies: %d", Monster_GetAliveCount());
    DrawText(enemyText, 10, 60, 20, WHITE);

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

// ---------------------------------------------------------------------------
// Build blocking map from TMX layers
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Spawn entities from TMX map objects
// ---------------------------------------------------------------------------
static void SpawnEntitiesFromObjects(Game* game) {
    if (!game || !game->map) return;

    for (int i = 0; i < game->map->objectCount; i++) {
        MapObject* obj = &game->map->objects[i];
        int tileX = (int)(obj->x / game->map->tileWidth);
        int tileY = (int)(obj->y / game->map->tileHeight);

        // --- Player spawn ----------------------------------------------------
        if (strcmp(obj->type, "player") == 0 || strcmp(obj->name, "player") == 0 ||
            strcmp(obj->type, "Player") == 0) {
            game->player.x = tileX;
            game->player.y = tileY;
            game->player.prevX = tileX;
            game->player.prevY = tileY;
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

// ---------------------------------------------------------------------------
// Initialize game
// ---------------------------------------------------------------------------
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

    // Try to load tileset texture
    if (game->map->tilesetCount > 0) {
        char imgPath[512] = {0};
        if (tmxFile) {
            const char* lastSlash = strrchr(tmxFile, '/');
            const char* lastBackslash = strrchr(tmxFile, '\\');
            const char* sep = (lastSlash > lastBackslash) ? lastSlash : lastBackslash;
            if (sep) {
                int dirLen = (int)(sep - tmxFile) + 1;
                snprintf(imgPath, sizeof(imgPath), "%.*s%s", dirLen, tmxFile, game->map->tilesets[0].imageSource);
            } else {
                snprintf(imgPath, sizeof(imgPath), "%s", game->map->tilesets[0].imageSource);
            }
        }
        if (imgPath[0] == '\0' || !FileExists(imgPath)) {
            snprintf(imgPath, sizeof(imgPath), "resources/%s", game->map->tilesets[0].imageSource);
        }

        TraceLog(LOG_INFO, "Loading tileset image: %s", imgPath);
        game->tilesetTexture = LoadTexture(imgPath);

        if (game->tilesetTexture.id == 0) {
            TraceLog(LOG_WARNING, "Could not load tileset texture: %s", imgPath);
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
            game->tilesetTexture = LoadTextureFromImage(img);
            UnloadImage(img);
            game->map->tilesets[0].columns = 8;
            game->map->tilesets[0].imageWidth  = game->map->tileWidth * 8;
            game->map->tilesets[0].imageHeight = game->map->tileHeight * 8;
        }
    }

    // Initialize player defaults (overridden by map objects below)
    game->player.x = 1;
    game->player.y = 1;
    game->player.prevX = 1;
    game->player.prevY = 1;
    strcpy(game->player.name, "Hero");
    game->player.hp = 20;
    game->player.maxHp = 20;
    game->player.attack = 5;
    game->player.defense = 1;
    game->player.level = 1;
    game->player.alive = true;
    game->player.isPlayer = true;
    game->player.facingRight = true;
    game->player.color = (Color){ 50, 200, 255, 255 };

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

    // Full visibility (no fog of war)
    for (int y = 0; y < game->map->height; y++) {
        for (int x = 0; x < game->map->width; x++) {
            game->visibility[y][x] = 1;
        }
    }

    // Setup camera
    game->camera.target = (Vector2){
        game->player.x * game->map->tileWidth  + game->map->tileWidth  / 2,
        game->player.y * game->map->tileHeight + game->map->tileHeight / 2
    };
    game->camera.offset = (Vector2){ GetScreenWidth() / 2, GetScreenHeight() / 2 };
    game->camera.rotation = 0;
    game->camera.zoom = 4.0f;

    return true;
}

// ---------------------------------------------------------------------------
// Cleanup game
// ---------------------------------------------------------------------------
void CleanupGame(Game* game) {
    if (!game) return;

    Monster_UnloadSprites();
    Monster_ResetAll();

    if (game->tilesetTexture.id > 0) UnloadTexture(game->tilesetTexture);
    if (game->map) {
        UnloadTMX(game->map);
        game->map = NULL;
    }

    memset(game, 0, sizeof(Game));
}
