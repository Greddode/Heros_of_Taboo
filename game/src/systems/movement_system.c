#include "movement_system.h"
#include "systems/world_monster.h"

bool MovementSystem_IsWalkable(const GameWorld* gw, int x, int y) {
    if (!gw || !gw->map) return false;
    if (x < 0 || x >= gw->map->width || y < 0 || y >= gw->map->height) return false;
    if (gw->blocking[y][x]) return false;
    if (World_FindMonsterAt(gw, x, y, ENTITY_NONE) != ENTITY_NONE) return false;
    return true;
}