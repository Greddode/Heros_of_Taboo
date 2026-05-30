#include "world.h"
#include <string.h>

void GameWorld_Init(GameWorld* gw) {
    memset(gw, 0, sizeof(GameWorld));
    World_Init(&gw->ecs);
    gw->playerEntity = ENTITY_NONE;
    gw->selectedMonsterEntity = ENTITY_NONE;
}
