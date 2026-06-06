#include "spatial_hash.h"
#include "raylib.h"
#include <string.h>

void SpatialHash_Clear(GameWorld* gw) {
    if (!gw) return;
    memset(gw->monsterGrid, 0, sizeof(gw->monsterGrid));
}

void SpatialHash_Add(GameWorld* gw, EntityId e, int x, int y) {
    if (!gw || e == ENTITY_NONE) return;
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        TraceLog(LOG_WARNING, "SpatialHash_Add: out of bounds [e=%d x=%d y=%d]", (int)e, x, y);
        return;
    }
    if (gw->monsterGrid[y][x] != ENTITY_NONE) {
        TraceLog(LOG_WARNING, "SpatialHash_Add: cell already occupied [x=%d y=%d old=%d new=%d]", x, y, (int)gw->monsterGrid[y][x], (int)e);
    }
    gw->monsterGrid[y][x] = e;
}

void SpatialHash_Remove(GameWorld* gw, EntityId e, int x, int y) {
    (void)e;
    if (!gw) return;
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        TraceLog(LOG_WARNING, "SpatialHash_Remove: out of bounds [e=%d x=%d y=%d]", (int)e, x, y);
        return;
    }
    if (gw->monsterGrid[y][x] != e) {
        TraceLog(LOG_WARNING, "SpatialHash_Remove: desync — cell contains %d, expected %d [x=%d y=%d]", (int)gw->monsterGrid[y][x], (int)e, x, y);
    }
    if (gw->monsterGrid[y][x] == e)
        gw->monsterGrid[y][x] = ENTITY_NONE;
}

void SpatialHash_Move(GameWorld* gw, EntityId e, int oldX, int oldY, int newX, int newY) {
    if (!gw || e == ENTITY_NONE) return;
    if (oldX >= 0 && oldX < MAP_WIDTH && oldY >= 0 && oldY < MAP_HEIGHT) {
        if (gw->monsterGrid[oldY][oldX] != e) {
            TraceLog(LOG_WARNING, "SpatialHash_Move: old cell desync — expected %d, found %d [old=(%d,%d)]", (int)e, (int)gw->monsterGrid[oldY][oldX], oldX, oldY);
        }
        if (gw->monsterGrid[oldY][oldX] == e)
            gw->monsterGrid[oldY][oldX] = ENTITY_NONE;
    }
    if (newX >= 0 && newX < MAP_WIDTH && newY >= 0 && newY < MAP_HEIGHT) {
        gw->monsterGrid[newY][newX] = e;
    }
}
