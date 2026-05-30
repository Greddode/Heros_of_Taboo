#include "game.h"
#include "data/monster_data.h"
#include "systems/spawner_system.h"
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

void RevealFOW(GameWorld* game) {
    int px = (game->playerEntity != ENTITY_NONE)
        ? World_GetPosition(&game->ecs, game->playerEntity)->x
        : 0;
    int py = (game->playerEntity != ENTITY_NONE)
        ? World_GetPosition(&game->ecs, game->playerEntity)->y
        : 0;

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
            if (game->blocking[ny][nx]) continue;

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

Vector2 TileToScreen(int x, int y, int tw, int th) {
    return (Vector2){ (float)(x * tw), (float)(y * th) };
}

void SpawnEscapeTile(GameWorld* game) {
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

void SpawnShadow(GameWorld* game) {
    int px = (game->playerEntity != ENTITY_NONE)
        ? World_GetPosition(&game->ecs, game->playerEntity)->x
        : 0;
    int py = (game->playerEntity != ENTITY_NONE)
        ? World_GetPosition(&game->ecs, game->playerEntity)->y
        : 0;
    int w = game->map->width;
    int h = game->map->height;
    int playerLevel = (game->playerEntity != ENTITY_NONE)
        ? World_GetStats(&game->ecs, game->playerEntity)->level
        : 1;

    for (int attempt = 0; attempt < 100; attempt++) {
        int tx = GetRandomValue(3, w - 4);
        int ty = GetRandomValue(3, h - 4);
        int dx = tx - px;
        int dy = ty - py;
        if (dx * dx + dy * dy >= 25 && !game->blocking[ty][tx] && IsFloorGID(GetTileGID(game->map, 0, tx, ty))) {
            EntityId shadow = SpawnerSystem_SpawnMonster(game, MONSTER_SHADOW, tx, ty, 1);
            if (shadow != ENTITY_NONE) {
                SpawnerSystem_ConfigureShadow(game, shadow, playerLevel);
                TraceLog(LOG_INFO, "Shadow spawned at (%d,%d)", tx, ty);
            }
            return;
        }
    }
    TraceLog(LOG_WARNING, "Shadow: could not find valid spawn position");
}

void BuildBlockingMap(GameWorld* game) {
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

void SpawnEntitiesFromObjects(GameWorld* game) {
    if (!game || !game->map) return;

    for (int i = 0; i < game->map->objectCount; i++) {
        MapObject* obj = &game->map->objects[i];
        int tileX = (int)(obj->x / game->map->tileWidth);
        int tileY = (int)(obj->y / game->map->tileHeight);

        // Player spawn
        if (strcmp(obj->type, "player") == 0 || strcmp(obj->name, "player") == 0 ||
            strcmp(obj->type, "Player") == 0) {
            if (game->playerEntity != ENTITY_NONE) {
                CPosition* pp = World_GetPosition(&game->ecs, game->playerEntity);
                pp->x = tileX;
                pp->y = tileY;
                pp->prevX = tileX;
                pp->prevY = tileY;
            }
            TraceLog(LOG_INFO, "Player spawned at (%d, %d)", tileX, tileY);
        }
        // Health potion
        else if (strcmp(obj->type, "healing") == 0 || strcmp(obj->type, "Healing") == 0 ||
                  strcmp(obj->type, "health") == 0 || strcmp(obj->type, "Health") == 0 ||
                  strcmp(obj->type, "health_potion") == 0 ||
                  strcmp(obj->name, "health_potion") == 0 || strcmp(obj->name, "HealthPotion") == 0) {
            SpawnerSystem_SpawnHealingPotionAt(game, tileX, tileY);
            TraceLog(LOG_INFO, "Health potion at (%d, %d)", tileX, tileY);
        }
        // Monsters
        else {
            EntityId mon = SpawnerSystem_SpawnByTypeName(game, obj->type, tileX, tileY, game->currentFloor);
            if (mon != ENTITY_NONE) {
                CName* n = World_GetName(&game->ecs, mon);
                TraceLog(LOG_INFO, "%s spawned at (%d, %d)", n ? n->name : "monster", tileX, tileY);
            }
        }
    }
}
