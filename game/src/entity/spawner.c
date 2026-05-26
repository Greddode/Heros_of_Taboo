#include "spawner.h"
#include "monster.h"
#include <stdlib.h>

// Minimum squared distance from the player's spawn for newly placed entities
#define MIN_PLAYER_DIST 5

static int DistSq(int ax, int ay, int bx, int by) {
    int dx = ax - bx;
    int dy = ay - by;
    return dx * dx + dy * dy;
}

// Populate each room with monsters and occasional healing items.
// Uses shuffled floor tile indices so placement is randomised.
void Spawner_Populate(Game* game, const ProceduralRoom* rooms, int roomCount) {
    if (!game || !game->map || roomCount == 0) return;

    int* tiles = game->map->layers[0].data;
    int w = game->map->width;
    int h = game->map->height;

    int px = game->player.ent.x;
    int py = game->player.ent.y;
    int minDist = MIN_PLAYER_DIST * MIN_PLAYER_DIST;

    // Upper bound: each room is at most 10x10 = 100 tiles
    #define ROOM_MAX_FLOORS 256

    for (int r = 0; r < roomCount; r++) {
        // Collect all floor-tile indices inside this room
        int floorTiles[ROOM_MAX_FLOORS];
        int floorCount = 0;

        for (int y = rooms[r].y; y < rooms[r].y + rooms[r].h; y++)
            for (int x = rooms[r].x; x < rooms[r].x + rooms[r].w; x++)
                if (IsFloorGID(tiles[y * w + x]) && tiles[y * w + x] != TILE_STAIRS && floorCount < ROOM_MAX_FLOORS)
                    floorTiles[floorCount++] = y * w + x;

        if (floorCount < 4) continue;

        // Fisher-Yates shuffle so placement order is random
        for (int i = floorCount - 1; i > 0; i--) {
            int j = GetRandomValue(0, i);
            int t = floorTiles[i]; floorTiles[i] = floorTiles[j]; floorTiles[j] = t;
        }

        // --- Monsters: bigger rooms get more enemies ---
        int monsterCount = 1;
        if (floorCount >= 20) monsterCount = 2;
        if (floorCount >= 40) monsterCount = 3;

        if (monsterCount > floorCount) monsterCount = floorCount;

        for (int m = 0; m < monsterCount; m++) {
            int mx = floorTiles[m] % w;
            int my = floorTiles[m] / w;

            // Don't spawn too close to the player
            if (DistSq(mx, my, px, py) < minDist) continue;
            if (Monster_GetAt(mx, my, NULL)) continue;

            // Weighted random monster selection, filtered by floor depth
            MonsterType type = MONSTER_BAT; // fallback
            float totalWeight = 0;
            float monsterWeights[MONSTER_TYPE_COUNT];
            int floor = game->currentFloor;

            for (int t = 0; t < MONSTER_TYPE_COUNT; t++) {
                const MonsterTemplate* tpl = Monster_GetTemplate((MonsterType)t);
                if (!tpl || tpl->spawnWeight <= 0 || floor < tpl->minFloor) {
                    monsterWeights[t] = 0;
                    continue;
                }
                monsterWeights[t] = (float)tpl->spawnWeight;
                totalWeight += monsterWeights[t];
            }

            if (totalWeight > 0) {
                float roll = (float)GetRandomValue(0, (int)(totalWeight * 100)) / 100.0f;
                float acc = 0;
                for (int t = 0; t < MONSTER_TYPE_COUNT; t++) {
                    if (monsterWeights[t] <= 0) continue;
                    acc += monsterWeights[t];
                    if (roll <= acc) {
                        type = (MonsterType)t;
                        break;
                    }
                }
            }

            Monster_Spawn(type, mx, my, floor);
        }

        // --- Healing item: only in larger rooms, placed on the last shuffled tile ---
        if (floorCount >= 40 && game->healingCount < MAX_HEALING) {
            int fi = floorCount - 1;
            int hx = floorTiles[fi] % w;
            int hy = floorTiles[fi] / w;
            if (DistSq(hx, hy, px, py) >= minDist) {
                game->healingTiles[game->healingCount][0] = hx;
                game->healingTiles[game->healingCount][1] = hy;
                game->healingCollected[game->healingCount] = false;
                game->healingCount++;
            }
        }
    }

    #undef ROOM_MAX_FLOORS

    TraceLog(LOG_INFO, "Spawner: %d monsters, %d healing items placed",
             Monster_GetAliveCount(), game->healingCount);
}
