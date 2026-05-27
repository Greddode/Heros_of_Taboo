#include "game.h"
#include "entity/spawner.h"
#include "entity/monster.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool IsInRoom(int x, int y) {
    ProceduralRoom rooms[MAX_GENERATED_ROOMS];
    int count = GetGeneratedRooms(rooms, MAX_GENERATED_ROOMS);
    for (int i = 0; i < count; i++) {
        if (x >= rooms[i].x && x < rooms[i].x + rooms[i].w &&
            y >= rooms[i].y && y < rooms[i].y + rooms[i].h)
            return true;
    }
    return false;
}

void RevealFOW(Game* game) {
    int px = game->player.ent.x;
    int py = game->player.ent.y;

    for (int y = 0; y < game->map->height; y++) {
        for (int x = 0; x < game->map->width; x++) {
            if (game->visibility[y][x] == 1)
                game->visibility[y][x] = 2;
        }
    }

    // Step 1: cast Bresenham rays — mark tiles with a clear line
    int r2 = FOG_RADIUS * FOG_RADIUS;
    for (int dy = -FOG_RADIUS; dy <= FOG_RADIUS; dy++) {
        int ny = py + dy;
        if (ny < 0 || ny >= game->map->height) continue;
        for (int dx = -FOG_RADIUS; dx <= FOG_RADIUS; dx++) {
            int nx = px + dx;
            if (nx < 0 || nx >= game->map->width) continue;
            if (dx * dx + dy * dy > r2) continue;
            if (nx == px && ny == py) { game->visibility[py][px] = 1; continue; }

            int ldx = abs(nx - px);
            int ldy = abs(ny - py);
            int steps = ldx > ldy ? ldx : ldy;
            int sx = (px < nx) ? 1 : -1;
            int sy = (py < ny) ? 1 : -1;
            int err = ldx - ldy;
            int bx = px, by = py;

            bool blocked = false;
            for (int i = 0; i < steps; i++) {
                int e2 = 2 * err;
                if (e2 > -ldy) { err -= ldy; bx += sx; }
                if (e2 < ldx)  { err += ldx; by += sy; }
                if ((bx != nx || by != ny) && game->blocking[by][bx]) {
                    blocked = true;
                    break;
                }
            }
            if (!blocked)
                game->visibility[ny][nx] = 1;
        }
    }

    // Step 2: wall adjacency — mark walls next to visible floor tiles only
    for (int dy = -FOG_RADIUS; dy <= FOG_RADIUS; dy++) {
        int ny = py + dy;
        if (ny < 0 || ny >= game->map->height) continue;
        for (int dx = -FOG_RADIUS; dx <= FOG_RADIUS; dx++) {
            int nx = px + dx;
            if (nx < 0 || nx >= game->map->width) continue;
            if (dx * dx + dy * dy > r2) continue;
            if (game->visibility[ny][nx] != 1) continue;
            if (game->blocking[ny][nx]) continue;  // only extend from floor tiles

            // Cardinal neighbors (4-dir) — always check
            static const int card[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
            bool isCorner = false;
            for (int n = 0; n < 4; n++) {
                int wx = nx + card[n][0];
                int wy = ny + card[n][1];
                if (wx < 0 || wx >= game->map->width || wy < 0 || wy >= game->map->height)
                    continue;
                if (game->visibility[wy][wx] == 1) continue;
                if (game->blocking[wy][wx]) {
                    game->visibility[wy][wx] = 1;
                    isCorner = true;
                }
            }

            // Diagonal neighbors — 8-dir check only on corner tiles
            // (tiles that have at least one adjacent wall to peek around)
            if (isCorner) {
                static const int diag[4][2] = {{-1,-1},{1,-1},{-1,1},{1,1}};
                for (int n = 0; n < 4; n++) {
                    int wx = nx + diag[n][0];
                    int wy = ny + diag[n][1];
                    if (wx < 0 || wx >= game->map->width || wy < 0 || wy >= game->map->height)
                        continue;
                    if (game->visibility[wy][wx] == 1) continue;
                    if (!game->blocking[wy][wx]) continue;
                    int cx = nx + diag[n][0];
                    int cy = ny;
                    int c2x = nx;
                    int c2y = ny + diag[n][1];
                    if (cx >= 0 && cx < game->map->width &&
                        c2y >= 0 && c2y < game->map->height &&
                        game->blocking[cy][cx] && game->blocking[c2y][c2x])
                        game->visibility[wy][wx] = 1;
                }
            }
        }
    }
}

void SpawnEscapeTile(Game* game) {
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

void SpawnShadow(Game* game) {
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
        float scale = powf(1.12f, (float)(targetLevel - 1));
        shadow->str      = (int)(4 * scale);
        shadow->dex      = (int)(6 * scale);
        shadow->intel    = (int)(2 * scale);
        shadow->con      = (int)(3 * scale);
        shadow->lck      = (int)(5 * scale);
        shadow->level    = targetLevel;
        shadow->maxHp    = 10 + shadow->con * 5;
        shadow->hp       = shadow->maxHp;
        shadow->attack   = 4 + shadow->str * 2;
        shadow->defense  = 1 + shadow->con / 2;
        shadow->expValue = 10 + shadow->lck * 3;
        shadow->shadowTurnCounter = 0;
        TraceLog(LOG_INFO, "Shadow spawned at (%d,%d) level %d", tx, ty, targetLevel);
    }
            return;
        }
    }
    TraceLog(LOG_WARNING, "Shadow: could not find valid spawn position");
}

void BuildBlockingMap(Game* game) {
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

void SpawnEntitiesFromObjects(Game* game) {
    if (!game || !game->map) return;

    for (int i = 0; i < game->map->objectCount; i++) {
        MapObject* obj = &game->map->objects[i];
        int tileX = (int)(obj->x / game->map->tileWidth);
        int tileY = (int)(obj->y / game->map->tileHeight);

        // Player spawn
        if (strcmp(obj->type, "player") == 0 || strcmp(obj->name, "player") == 0 ||
            strcmp(obj->type, "Player") == 0) {
            game->player.ent.x = tileX;
            game->player.ent.y = tileY;
            game->player.ent.prevX = tileX;
            game->player.ent.prevY = tileY;
            TraceLog(LOG_INFO, "Player spawned at (%d, %d)", tileX, tileY);
        }
            // Health potion
        else if (strcmp(obj->type, "healing") == 0 || strcmp(obj->type, "Healing") == 0 ||
                  strcmp(obj->type, "health") == 0 || strcmp(obj->type, "Health") == 0 ||
                  strcmp(obj->type, "health_potion") == 0 ||
                  strcmp(obj->name, "health_potion") == 0 || strcmp(obj->name, "HealthPotion") == 0) {
            if (game->potionCount >= MAX_POTIONS) continue;
            game->potionTiles[game->potionCount][0] = tileX;
            game->potionTiles[game->potionCount][1] = tileY;
            game->potionCollected[game->potionCount] = false;
            game->potionQuantities[game->potionCount] = 1;
            
            // Random roll for potion rarity: 50% small, 30% medium, 20% large
            int roll = GetRandomValue(1, 100);
            if (roll <= 50) {
                game->potionTypes[game->potionCount] = ITEM_SMALL_HP_POTION;
            } else if (roll <= 80) { // 50+30=80
                game->potionTypes[game->potionCount] = ITEM_MEDIUM_HP_POTION;
            } else {
                game->potionTypes[game->potionCount] = ITEM_LARGE_HP_POTION;
            }
            game->potionCount++;
            TraceLog(LOG_INFO, "Health potion at (%d, %d)", tileX, tileY);
        }
        // Monsters
        else {
            Monster* mon = Monster_SpawnByTypeName(obj->type, tileX, tileY, game->currentFloor);
            if (mon) TraceLog(LOG_INFO, "%s spawned at (%d, %d)", mon->name, tileX, tileY);
        }
    }
}
