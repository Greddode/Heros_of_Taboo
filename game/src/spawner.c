#include "spawner.h"
#include "monster.h"
#include <stdlib.h>

#define MIN_PLAYER_DIST 5

static int DistSq(int ax, int ay, int bx, int by) {
    int dx = ax - bx;
    int dy = ay - by;
    return dx * dx + dy * dy;
}

void Spawner_Populate(Game* game, const ProceduralRoom* rooms, int roomCount) {
    if (!game || !game->map || roomCount == 0) return;

    int* tiles = game->map->layers[0].data;
    int w = game->map->width;
    int h = game->map->height;

    int px = game->player.x;
    int py = game->player.y;
    int minDist = MIN_PLAYER_DIST * MIN_PLAYER_DIST;

    for (int r = 0; r < roomCount; r++) {
        // Collect floor tiles in this room
        int floorTiles[256];
        int floorCount = 0;

        for (int y = rooms[r].y; y < rooms[r].y + rooms[r].h; y++)
            for (int x = rooms[r].x; x < rooms[r].x + rooms[r].w; x++)
                if (tiles[y * w + x] == TILE_FLOOR)
                    floorTiles[floorCount++] = y * w + x;

        if (floorCount < 4) continue;

        // Shuffle floor tiles
        for (int i = floorCount - 1; i > 0; i--) {
            int j = GetRandomValue(0, i);
            int t = floorTiles[i]; floorTiles[i] = floorTiles[j]; floorTiles[j] = t;
        }

        // --- Monsters ---
        int monsterCount = 1;
        if (floorCount >= 20) monsterCount = 2;
        if (floorCount >= 40) monsterCount = 3;

        for (int m = 0; m < monsterCount; m++) {
            if (m >= floorCount) break;

            int mx = floorTiles[m] % w;
            int my = floorTiles[m] / w;

            if (DistSq(mx, my, px, py) < minDist) continue;
            Monster* existing = Monster_GetAt(mx, my, NULL);
            if (existing && existing->alive) continue;

            MonsterType type;
            int roll = GetRandomValue(0, 100);
            if (roll < 40)      type = MONSTER_FLOATING_EYE;
            else if (roll < 75) type = MONSTER_FUNGAL_MYCONID;
            else                type = MONSTER_OGRE;

            Monster_Spawn(type, mx, my);
        }

        // --- Healing item (rare) ---
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

    TraceLog(LOG_INFO, "Spawner: %d monsters, %d healing items placed",
             Monster_GetAliveCount(), game->healingCount);
}
